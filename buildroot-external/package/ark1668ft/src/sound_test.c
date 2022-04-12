#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#include "alsa/asoundlib.h"
#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

#define RECORD_BYTES        16000 * 2 * 2 * 3 / 2 //record 1.5s data
static pthread_t play_tid;

static int set_alsa_paramters(snd_pcm_t *handle, int rate, int channels, int bits_per_sample)
{
	int ret;
	unsigned int val;
	int dir = 0;
	char *buffer;
	int size;
	snd_pcm_uframes_t buffer_size, period_size;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_t *sw_params;
	unsigned period_time = 0, buffer_time = 0;
	snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;

	switch(bits_per_sample) {
	case 8:
		format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		format = SND_PCM_FORMAT_U24_LE;
		break;
	case 32:
		format = SND_PCM_FORMAT_U32_LE;
		break;
	default:
		format = SND_PCM_FORMAT_U8;
		break;
	}

	ret = snd_pcm_hw_params_malloc(&hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_malloc");
		return -1;
	}

	ret = snd_pcm_hw_params_any(handle, hw_params);
	if (ret < 0) {
		perror("snd_pcm_hw_params_any");
		return -1;
	}
	ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_access");
		return -1;
	}
	ret = snd_pcm_hw_params_set_format(handle, hw_params, format);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_format");
		return -1;
	}

	ret = snd_pcm_hw_params_set_channels(handle, hw_params, channels);
	printf("ret = %d ++++++++++++++  \n", ret);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_channels");
		return -1;
	}

	val = rate;
	ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &val, &dir);
	if (ret < 0) {
		perror("snd_pcm_hw_params_set_rate_near");
		return -1;
	}

    /* period_size = RECORD_BYTES / 4;
    ret = snd_pcm_hw_params_set_period_size_near(handle, hw_params, &period_size, &dir);
 	if (ret < 0) {
		perror("snd_pcm_hw_params_set_period_size_near");
		return -1;
	} */

	ret = snd_pcm_hw_params(handle, hw_params);
	if (ret < 0) {
		printf("unable to set hw parameters: %s\n", snd_strerror(ret));
		return -1;
	}

	snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
	snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	printf("%s:%d period_size=%d buffer_size=%d \n", __func__, __LINE__, period_size, buffer_size);
	if (period_size == buffer_size) {
		printf("Can't use period equal to buffer size (%lu == %lu)",
		      period_size, buffer_size);
		return -1;
	}
	if (hw_params)
		snd_pcm_hw_params_free(hw_params);
	return period_size;
}

/*

This example reads from the default PCM device
and writes to standard output for 5 seconds of data.

*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

/**************************************************************/
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT 0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

static uint32_t totle_size = 0;

struct wav_header {
    /* RIFF WAVE Chunk */
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    /* Format Chunk */
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; /* sample_rate * num_channels * bps / 8 */
    uint16_t block_align; /* num_channels * bps / 8 */
    uint16_t bits_per_sample;
    uint16_t reserved;
    /* Data Chunk */
    uint32_t data_id;
    uint32_t data_sz;
}__attribute__((packed));

static struct wav_header hdr;

/**************************************************************/
int playback_file(unsigned rate, uint16_t channels, int fd, uint32_t total_count)
{
    int rc;
    int size;
    int left_size = total_count;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int dir;
    snd_pcm_uframes_t frames;
    char *buffer;                    /* TODO */

    /* Open PCM device for playbacking. */
    rc = snd_pcm_open(&handle, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        printf("unable to open pcm device: %s\n", snd_strerror(rc));
        exit(1);
    }

    frames = set_alsa_paramters(handle, rate, channels, 16);

    size = frames * 2 * channels; /* 2 bytes/sample(16bit), 2 channels */
    buffer = (char *) malloc(size);

    while (left_size > 0) {
        rc = read(fd, buffer, size);
        if (rc != size)
            printf("short read: read %d bytes\n", rc);
        left_size -= rc;

        rc = snd_pcm_writei(handle, buffer, rc / 2 / channels);
        if (rc == -EPIPE) {
            /* EPIPE means overrun */
            printf("overrun occurred\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            printf("error from write: %s\n", snd_strerror(rc));
        } else if (rc != (int)frames) {
            printf("short write, write %d frames\n", rc);
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);

    return 0;
}

/**************************************************************/
int play_wav(const char *fn)
{
    unsigned rate, channels;
    int fd;

    fd = open(fn, O_RDONLY, 0777);
    if (fd < 0) {
        printf("playback: cannot open '%s'\n", fn);
        return -1;
    }

    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        printf("playback: cannot read header\n");
        return -1;
    }
    printf("playback: %d ch, %d hz, %d bit, %s, file_size %ld\n",
            hdr.num_channels, hdr.sample_rate, hdr.bits_per_sample,
            hdr.audio_format == FORMAT_PCM ? "PCM" : "unknown", hdr.data_sz);

    return playback_file(hdr.sample_rate, hdr.num_channels, fd, hdr.data_sz);
}

void *playback_thread(void *arg)
{
    play_wav(PLAYBACK_AUDIO_PATH);
}

static inline int is_match(short *buf, int start, int dir)
{
    int i;

    for (i = start; i < start + 16 - 2; i+= 2) {
        if (dir && buf[i] >= buf[i + 2])
            return 0;
        if (!dir && buf[i] <= buf[i + 2])
            return 0;
    }

    return 1;
}

void *sound_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    snd_pcm_t *handle = NULL;
    snd_pcm_uframes_t frames;
    short *revbuf = NULL;
    char *tmp = NULL;
    int revlen = RECORD_BYTES;
    int top;
    int ret;
    int i, j;

    pthread_create(&play_tid, NULL, playback_thread, NULL);
	pthread_detach(play_tid);

    if (snd_pcm_open(&handle, "hw:0,1", SND_PCM_STREAM_CAPTURE, O_NONBLOCK) < 0) {
        printf("snd_pcm_open fail\n");
        goto err;
    }

    if ((frames = set_alsa_paramters(handle, 16000, 2, 16)) < 0) {
        printf("set_alsa_paramters err.\n");
        goto err;
    }

    revbuf = malloc(RECORD_BYTES);
    if (!revbuf) {
        printf("no enough memory to malloc.\n");
        goto err;
    }
    tmp = (char*)revbuf;

    while(revlen > 0) {
        int len = revlen > frames * 4 ? frames : revlen / 4;
        ret = snd_pcm_readi(handle, tmp, len);
        if (ret == -EPIPE) {
            /* EPIPE means overrun */
            printf("overrun occurred\n");
            snd_pcm_prepare(handle);
            continue;
        } else if (ret < 0) {
            printf("error from read: %s\n", snd_strerror(ret));
            goto err;
        } else if (ret != (int)len) {
            printf("short read, read %d frames\n", ret);
        }
        revlen -= ret * 4;
        tmp += ret * 4;
    }

    for (i = 0; i < RECORD_BYTES; i += 2) {
        for (j = 0; j < SOUND_MATCH; j++) {
            if (!is_match(revbuf, i + j * 16, j & 1))
                break;
        }

        if (j == SOUND_MATCH)
            break;
    }

    if (i == RECORD_BYTES) {
        int fd_pcm = open("1.pcm", O_WRONLY | O_CREAT | O_TRUNC);
        write(fd_pcm, revbuf, RECORD_BYTES);
        close(fd_pcm);
        goto err;
    }

    free(revbuf);
    snd_pcm_close(handle);

    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (revbuf)
        free(revbuf);
    if (handle)
        snd_pcm_close(handle);
    rt->finish = 1;
    return (void*)-1;
}