#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <linux/input.h>

const struct input_event
syn = {.type = EV_SYN, .code = SYN_REPORT, .value = 0},
enter_up = {.type = EV_KEY, .code = KEY_ENTER, .value = 0},
ctrl_up = {.type = EV_KEY, .code = KEY_RIGHTCTRL, .value = 0},
enter_down = {.type = EV_KEY, .code = KEY_ENTER, .value = 1},
ctrl_down = {.type = EV_KEY, .code = KEY_RIGHTCTRL, .value = 1};

void print_usage(FILE *stream, const char *program) {
	fprintf(stream,
		"enter2ctrl - hold enter to get right control.\n"
		"\n"
		"usage: %s [-h | [-t delay]]\n"
		"\n"
		"options:\n"
		"	-h	 show this message and exit\n"
		"	-t	 delay used for key sequences (default: 20000 microseconds)\n",
		program);
}

int read_event(struct input_event *event) {
	return fread(event, sizeof(struct input_event), 1, stdin) == 1;
}

void write_event(const struct input_event *event) {
	if (fwrite(event, sizeof(struct input_event), 1, stdout) != 1)
		exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int delay = 20000;

	for (int opt; (opt = getopt(argc, argv, "ht:")) != -1;) {
		switch (opt) {
		case 'h':
			return print_usage(stdout, argv[0]), EXIT_SUCCESS;
		case 't':
			delay = atoi(optarg);
			continue;
		}
		return print_usage(stderr, argv[0]), EXIT_FAILURE;
	}

	struct input_event input;
	enum { START, ENTER_HELD, ENTER_IS_CTRL } state = START;

	setbuf(stdin, NULL), setbuf(stdout, NULL);

	while (read_event(&input)) {
		if (input.type == EV_MSC && input.code == MSC_SCAN)
			continue;

		if (input.type != EV_KEY && input.type != EV_REL &&
			input.type != EV_ABS) {
			write_event(&input);
			continue;
		}

		switch (state) {
		case START:
			if (input.type == EV_KEY && input.code == KEY_ENTER &&
				input.value)
				state = ENTER_HELD;
			else
				write_event(&input);
			break;
		case ENTER_HELD:
			if (input.type == EV_KEY && input.code == KEY_ENTER && !input.value) {
				write_event(&enter_down);
				write_event(&syn);
				usleep(delay);
				write_event(&enter_up);
				state = START;
			} else if ((input.type == EV_KEY && input.value == 1) ||
					   input.type == EV_REL || input.type == EV_ABS) {
				write_event(&ctrl_down);
				write_event(&syn);
				usleep(delay);
				write_event(&input);
				state = ENTER_IS_CTRL;
			} else {
				write_event(&input);
			}
			break;
		case ENTER_IS_CTRL:
			if (input.type == EV_KEY && input.code == KEY_ENTER) {
				input.code = KEY_RIGHTCTRL;
				write_event(&input);
				if (input.value == 0)
					state = START;
			} else {
				write_event(&input);
			}
			break;
		}
	}
}
