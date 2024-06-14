#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ftdi.h>

#define CHANNEL_COUNT 512

struct RBG
{
	float r, g, b;
};

struct RBG hsv_to_rgb(float h, float s, float v) {
	int i = h * 6;
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	switch(i % 6) {
		case 0: return (struct RBG){ v, t, p };
		case 1: return (struct RBG){ q, v, p };
		case 2: return (struct RBG){ p, v, t };
		case 3: return (struct RBG){ p, q, v };
		case 4: return (struct RBG){ t, p, v };
		case 5: return (struct RBG){ v, p, q };
	}

	/* should not happen */
	return (struct RBG){ 1, 1, 1 };
}

int main(int argc, char **argv)
{
	printf("hello world\n");

	const char *prog_name = "./a.out";

	if (0 < argc) {
		prog_name = argv[0];
	}

	if (argc < 2) {
		printf("usage: %s <device path>\n", argv[0]);
		return 1;
	}
	const char *device_path = argv[1];

	struct ftdi_context *ctx = ftdi_new();
	if (ctx == NULL) {
		printf("failed to allocate ftdi context\n");
		return 1;
	}

	int err = ftdi_usb_open_string(ctx, device_path);
	if (err) {
		printf("failed to open device: %d\n", err);
		return 1;
	}

	if (ftdi_set_baudrate(ctx, 250000)) {
		printf("failed to set baudrate\n");
		return 1;
	}

	for(int frame = 0;; frame++) {
		unsigned char channels[CHANNEL_COUNT];
		memset(channels, 0xff, CHANNEL_COUNT);

		unsigned char frame_byte = frame & 0xff;
		float frame_float = frame_byte;
		frame_float /= 0xff;

		struct RBG rgb = hsv_to_rgb(frame_float, 1.0, 1.0);

		for (int i = 0; i < 10; i++) {
			channels[0 + (i * 8)] = 0xff;
			channels[1 + (i * 8)] = (rgb.r) * 0xff;
			channels[2 + (i * 8)] = (rgb.g) * 0xff;
			channels[3 + (i * 8)] = (rgb.b) * 0xff;
			channels[4 + (i * 8)] = 0x00;
			channels[5 + (i * 8)] = 0x00;
			channels[6 + (i * 8)] = 0x00;
		}

		/* break */
		ftdi_set_line_property2(ctx, BITS_8, STOP_BIT_2, NONE, BREAK_ON);
		usleep(1000);
		ftdi_set_line_property2(ctx, BITS_8, STOP_BIT_2, NONE, BREAK_OFF);

		/* mark */
		ftdi_set_line_property2(ctx, BITS_8, STOP_BIT_2, MARK, BREAK_OFF);

		unsigned char start = 0;
		if (ftdi_write_data(ctx, &start, 1) != 1) {
			printf("failed to write start byte\n");
			return 1;
		}

		/* unmark */
		ftdi_set_line_property2(ctx, BITS_8, STOP_BIT_2, NONE, BREAK_OFF);

		/* send channels */
		if (ftdi_write_data(ctx, channels, CHANNEL_COUNT) != CHANNEL_COUNT) {
			printf("failed to write channels\n");
			return 1;
		}
		usleep(50000);
		printf("done\n");
	}

	return 0;
}
