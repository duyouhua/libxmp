#include "test.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#undef TEST
#include "../src/player.h"
#include "../src/mixer.h"
#include "../src/virtual.h"

static void _compare_mixer_data(char *mod, char *data, int loops, int ignore_rv)
{
	xmp_context opaque;
	struct context_data *ctx;
	struct module_data *m;
        struct player_data *p;
        struct mixer_data *s;
        struct mixer_voice *vi;
	struct xmp_frame_info fi;
	int time, row, frame, chan, period, note, ins, vol, pan, pos0, cutoff;
	char line[200];
	FILE *f;
	int i, voc;
	int ret;

	f = fopen(data, "r");
	fail_unless(f != NULL, "can't open data file");

	opaque = xmp_create_context();
	fail_unless(opaque != NULL, "can't create context");

	ret = load_module(opaque, mod);
	fail_unless(ret == 0, "can't load module");

	ctx = (struct context_data *)opaque;
	m = &ctx->m;
	p = &ctx->p;
	s = ctx->s;

	xmp_start_player(opaque, 44100, 0);
	xmp_set_player(opaque, XMP_PLAYER_MIX, 100);

	while (1) {
		xmp_play_frame(opaque);
		xmp_get_frame_info(opaque, &fi);
		if (fi.loop_count >= loops)
			break;

		for (i = 0; i < m->mod.chn; i++) {
			struct xmp_channel_info *ci = &fi.channel_info[i];
			struct channel_data *xc = &p->xc_data[i];
			int num;

			voc = map_channel(p, i);
			if (voc < 0 || TEST_NOTE(NOTE_SAMPLE_END))
				continue;

			vi = &s->voice[voc];

#if 1
			fgets(line, 200, f);
			num = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d",
				&time, &row, &frame, &chan, &period,
				&note, &ins, &vol, &pan, &pos0, &cutoff);

			fail_unless(fi.time    == time,   "time mismatch");
			fail_unless(fi.row     == row,    "row mismatch");
			fail_unless(fi.frame   == frame,  "frame mismatch");
			fail_unless(i          == chan,   "channel mismatch");
			fail_unless(ci->period == period, "period mismatch");
			fail_unless(vi->note   == note,   "note mismatch");
			fail_unless(vi->ins    == ins,    "instrument");
			if (!ignore_rv) {
				fail_unless(vi->vol == vol, "volume mismatch");
				fail_unless(vi->pan == pan, "pan mismatch");
			}
			fail_unless(vi->pos0   == pos0,   "position mismatch");
			if (num >= 11) {
				fail_unless(vi->filter.cutoff == cutoff,
							  "cutoff mismatch");
			}
#else
			fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d\n",
				fi.time, fi.row, fi.frame, i, ci->period,
				vi->note, vi->ins, vi->vol, vi->pan, vi->pos0, vi->filter.cutoff);
#endif
		}
		
	}

	fgets(line, 200, f);
	//fail_unless(feof(f), "not end of data file");

	xmp_end_player(opaque);
	xmp_release_module(opaque);
	xmp_free_context(opaque);
}

int load_module(xmp_context opaque, char *mod)
{
	int fd;
	void *addr;
	struct stat st;
	int ret;

	/* mmap mod file */
	fd = open(mod, O_RDONLY);
	fail_unless(fd > 0, "can't open mod file");

	ret = fstat(fd, &st);
	fail_unless(ret == 0, "can't stat mod file");

	addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	fail_unless(addr != MAP_FAILED, "can't mmap mod file");

	ret = xmp_load_module(opaque, addr, st.st_size);
	fail_unless(ret == 0, "can't load mod file");

	return ret;
}

void compare_mixer_data(char *mod, char *data)
{
	_compare_mixer_data(mod, data, 1, 0);
}

void compare_mixer_data_loops(char *mod, char *data, int loops)
{
	_compare_mixer_data(mod, data, loops, 0);
}

void compare_mixer_data_no_rv(char *mod, char *data)
{
	_compare_mixer_data(mod, data, 1, 1);
}
