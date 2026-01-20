#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>

#define INPUT_PATH "/dev/input"
#define UINPUT_PATH "/dev/uinput"
#define MAX_DEVICES 32

static int running = 1;
static int uinput_mouse_fd = -1;
static int uinput_kbd_fd = -1;

typedef struct {
    int fd;
    char path[1024];
    char name[256];
} Device;

static Device devices[MAX_DEVICES];
static int device_count = 0;

void cleanup(int sig) {
    (void)sig;
    running = 0;
}

int setup_virtual_mouse() {
    int fd = open(UINPUT_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return -1;

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    struct uinput_setup usetup = {0};
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "CapsLock Virtual Mouse");
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int setup_virtual_keyboard() {
    int fd = open(UINPUT_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return -1;

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_EVBIT, EV_MSC);
    ioctl(fd, UI_SET_EVBIT, EV_REP);
    
    for (int i = 0; i < KEY_MAX; i++) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }
    
    ioctl(fd, UI_SET_MSCBIT, MSC_SCAN);

    struct uinput_setup usetup = {0};
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "CapsLock Virtual Keyboard");
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5679;

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

void send_click(int fd, int press) {
    struct input_event ev = {0};
    
    gettimeofday(&ev.time, NULL);
    ev.type = EV_KEY;
    ev.code = BTN_LEFT;
    ev.value = press;
    write(fd, &ev, sizeof(ev));
    
    gettimeofday(&ev.time, NULL);
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

ssize_t reinject_event(int fd, struct input_event *ev) {
    return write(fd, ev, sizeof(*ev));
}

int has_capslock_key(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    unsigned long evbit = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
    
    int has_keys = evbit & (1 << EV_KEY);
    
    if (has_keys) {
        unsigned char key_bits[KEY_MAX/8 + 1] = {0};
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits);
        
        int has_capslock = key_bits[KEY_CAPSLOCK/8] & (1 << (KEY_CAPSLOCK % 8));
        close(fd);
        return has_capslock;
    }
    
    close(fd);
    return 0;
}

int find_all_keyboards(Device *devices, int max_devices) {
    DIR *dir = opendir(INPUT_PATH);
    if (!dir) return 0;

    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;
        
        char device_path[512];
        snprintf(device_path, sizeof(device_path), "%s/%s", INPUT_PATH, entry->d_name);
        
        if (has_capslock_key(device_path)) {
            int fd = open(device_path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                devices[count].fd = fd;
                snprintf(devices[count].path, sizeof(devices[count].path), "%s", device_path);
                ioctl(fd, EVIOCGNAME(sizeof(devices[count].name)), devices[count].name);
                
                // grab.
                ioctl(fd, EVIOCGRAB, 1);
                
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    device_count = find_all_keyboards(devices, MAX_DEVICES);
    if (device_count == 0) return 1;

    uinput_mouse_fd = setup_virtual_mouse();
    if (uinput_mouse_fd < 0) return 1;

    uinput_kbd_fd = setup_virtual_keyboard();
    if (uinput_kbd_fd < 0) {
        close(uinput_mouse_fd);
        return 1;
    }

    int caps_pressed = 0;
    struct input_event ev;
    int event_count = 0;

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        
        int max_fd = -1;
        for (int i = 0; i < device_count; i++) {
            FD_SET(devices[i].fd, &readfds);
            if (devices[i].fd > max_fd) max_fd = devices[i].fd;
        }

        struct timeval timeout = {1, 0};
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        for (int i = 0; i < device_count; i++) {
            if (!FD_ISSET(devices[i].fd, &readfds)) continue;
            
            ssize_t n = read(devices[i].fd, &ev, sizeof(ev));
            if (n != sizeof(ev)) continue;

            event_count++;

            if (ev.type == EV_KEY && ev.code == KEY_CAPSLOCK) {
                if (ev.value == 1 && !caps_pressed) {
                    send_click(uinput_mouse_fd, 1);
                    caps_pressed = 1;
                } else if (ev.value == 0 && caps_pressed) {
                    send_click(uinput_mouse_fd, 0);
                    caps_pressed = 0;
                }
            } else {
                if (ev.type != EV_LED) {
                    reinject_event(uinput_kbd_fd, &ev);
                }
            }
        }
    }


    // Clean.
    for (int i = 0; i < device_count; i++) {
        ioctl(devices[i].fd, EVIOCGRAB, 0);
        close(devices[i].fd);
    }
    
    if (uinput_mouse_fd >= 0) {
        ioctl(uinput_mouse_fd, UI_DEV_DESTROY);
        close(uinput_mouse_fd);
    }
    
    if (uinput_kbd_fd >= 0) {
        ioctl(uinput_kbd_fd, UI_DEV_DESTROY);
        close(uinput_kbd_fd);
    }

    return 0;
}