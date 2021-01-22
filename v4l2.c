/*
 * Copyright (c) 2014 Philippe De Muyter <phdm@macqel.be>
 * Copyright (c) 2014 William Manley <will@williammanley.net>
 * Copyright (c) 2011 Peter Zotov <whitequark@whitequark.org>
 * Copyright (c) 2014-2020 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "defs.h"

#include "static_assert.h"

#define CHECK_V4L2_STRUCT_SIZE(s_) \
	static_assert(sizeof(struct s_) == sizeof(struct_##s_), \
		      "Unexpected struct " #s_ " size")
#define CHECK_V4L2_STRUCT_SIZE_LE(s_) \
	static_assert(sizeof(struct s_) <= sizeof(struct_##s_), \
		      "Unexpected struct " #s_ " size, " \
		      "please update the decoder")
#define CHECK_V4L2_RESERVED_SIZE(s_) \
	static_assert(sizeof_field(struct s_, reserved) \
		      >= sizeof_field(struct_##s_, reserved), \
		      "Unexpected struct " #s_ ".reserved size, " \
		      "please update the decoder")
#define CHECK_V4L2_STRUCT_RESERVED_SIZE(s_) \
	CHECK_V4L2_STRUCT_SIZE(s_); \
	CHECK_V4L2_RESERVED_SIZE(s_)

#include DEF_MPERS_TYPE(kernel_v4l2_buffer_t)
#include DEF_MPERS_TYPE(kernel_v4l2_event_t)
#include DEF_MPERS_TYPE(kernel_v4l2_timeval_t)
#include DEF_MPERS_TYPE(struct_v4l2_clip)
#include DEF_MPERS_TYPE(struct_v4l2_create_buffers)
#include DEF_MPERS_TYPE(struct_v4l2_ext_control)
#include DEF_MPERS_TYPE(struct_v4l2_ext_controls)
#include DEF_MPERS_TYPE(struct_v4l2_format)
#include DEF_MPERS_TYPE(struct_v4l2_framebuffer)
#include DEF_MPERS_TYPE(struct_v4l2_input)
#include DEF_MPERS_TYPE(struct_v4l2_standard)
#include DEF_MPERS_TYPE(struct_v4l2_window)

#include "kernel_v4l2_types.h"

CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_capability);
CHECK_V4L2_STRUCT_SIZE_LE(v4l2_pix_format);
#ifdef HAVE_STRUCT_V4L2_PLANE_PIX_FORMAT
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_plane_pix_format);
#endif
#ifdef HAVE_STRUCT_V4L2_PIX_FORMAT_MPLANE
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_pix_format_mplane);
#endif
CHECK_V4L2_STRUCT_SIZE(v4l2_clip);
CHECK_V4L2_STRUCT_SIZE_LE(v4l2_window);
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_vbi_format);
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_sliced_vbi_format);
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_sliced_vbi_cap);
#ifdef HAVE_STRUCT_V4L2_SDR_FORMAT
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_sdr_format);
#endif
#ifdef HAVE_STRUCT_V4L2_META_FORMAT
CHECK_V4L2_STRUCT_SIZE(v4l2_meta_format);
#endif
CHECK_V4L2_STRUCT_SIZE(v4l2_format);
#ifdef HAVE_STRUCT_V4L2_QUERY_EXT_CTRL
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_query_ext_ctrl);
#endif
#ifdef HAVE_STRUCT_V4L2_FRMSIZEENUM
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_frmsizeenum);
#endif
#ifdef HAVE_STRUCT_V4L2_FRMIVALENUM
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_frmivalenum);
#endif
#ifdef HAVE_STRUCT_V4L2_CREATE_BUFFERS
CHECK_V4L2_STRUCT_RESERVED_SIZE(v4l2_create_buffers);
#endif

#include MPERS_DEFS

#include "print_fields.h"
#include "print_utils.h"
#include "xstring.h"

/* v4l2_fourcc_be was added by Linux commit v3.18-rc1~101^2^2~127 */
#ifndef v4l2_fourcc_be
# define v4l2_fourcc_be(a, b, c, d) (v4l2_fourcc(a, b, c, d) | (1U << 31))
#endif

#ifndef VIDEO_MAX_PLANES
# define VIDEO_MAX_PLANES 8
#endif

#include "xlat/v4l2_meta_fmts.h"
#include "xlat/v4l2_pix_fmts.h"
#include "xlat/v4l2_sdr_fmts.h"

#define XLAT_MACROS_ONLY
# include "xlat/v4l2_ioctl_cmds.h"
#undef XLAT_MACROS_ONLY

static void
print_v4l2_rect(const MPERS_PTR_ARG(struct v4l2_rect *) const arg)
{
	const struct v4l2_rect *const p = arg;
	PRINT_FIELD_D("{", *p, left);
	PRINT_FIELD_D(", ", *p, top);
	PRINT_FIELD_U(", ", *p, width);
	PRINT_FIELD_U(", ", *p, height);
	tprints("}");
}

#define PRINT_FIELD_FRACT(prefix_, where_, field_)			\
	STRACE_PRINTF("%s%s=%u/%u", (prefix_), #field_,			\
		      (where_).field_.numerator,			\
		      (where_).field_.denominator)

static void
print_pixelformat(uint32_t fourcc, const struct xlat *xlat)
{
	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW) {
		tprintf("%#x", fourcc);
		return;
	}

	unsigned char a[] = {
		(unsigned char) fourcc,
		(unsigned char) (fourcc >> 8),
		(unsigned char) (fourcc >> 16),
		(unsigned char) (fourcc >> 24),
	};
	unsigned int i;

	tprints("v4l2_fourcc(");
	/* Generic char array printing routine.  */
	for (i = 0; i < ARRAY_SIZE(a); ++i) {
		unsigned char c = a[i];

		if (i)
			tprints(", ");
		if (c == '\'' || c == '\\') {
			char sym[] = {
				'\'',
				'\\',
				c,
				'\'',
				'\0'
			};
			tprints(sym);
		} else if (is_print(c)) {
			char sym[] = {
				'\'',
				c,
				'\'',
				'\0'
			};
			tprints(sym);
		} else {
			char hex[] = {
				BYTE_HEX_CHARS_PRINTF_QUOTED(c),
				'\0'
			};
			tprints(hex);
		}
	}
	tprints(")");

	if (xlat) {
		const char *pixfmt_name = xlookup(xlat, fourcc);

		if (pixfmt_name)
			tprints_comment(pixfmt_name);
	}
}

#define PRINT_FIELD_PIXFMT(prefix_, where_, field_, xlat_)	\
	do {							\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);	\
		print_pixelformat((where_).field_, (xlat_));	\
	} while (0)

#include "xlat/v4l2_device_capabilities_flags.h"

static int
print_v4l2_capability(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_capability caps;

	if (entering(tcp))
		return 0;
	tprints(", ");
	if (umove_or_printaddr(tcp, arg, &caps))
		return RVAL_IOCTL_DECODED;
	PRINT_FIELD_CSTRING("{", caps, driver);
	PRINT_FIELD_CSTRING(", ", caps, card);
	PRINT_FIELD_CSTRING(", ", caps, bus_info);
	PRINT_FIELD_KERNEL_VERSION(", ", caps, version);
	PRINT_FIELD_FLAGS(", ", caps, capabilities,
			  v4l2_device_capabilities_flags, "V4L2_CAP_???");
	if (caps.device_caps) {
		PRINT_FIELD_FLAGS(", ", caps, device_caps,
				  v4l2_device_capabilities_flags,
				  "V4L2_CAP_???");
	}
	tprints("}");
	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_buf_types.h"
#include "xlat/v4l2_format_description_flags.h"

static int
print_v4l2_fmtdesc(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct v4l2_fmtdesc f;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &f))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", f, index);
		PRINT_FIELD_XVAL(", ", f, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &f)) {
		PRINT_FIELD_FLAGS(", ", f, flags,
				  v4l2_format_description_flags,
				  "V4L2_FMT_FLAG_???");
		PRINT_FIELD_CSTRING(", ", f, description);
		PRINT_FIELD_PIXFMT(", ", f, pixelformat, v4l2_pix_fmts);
	}
	tprints("}");
	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_fields.h"
#include "xlat/v4l2_colorspaces.h"
#include "xlat/v4l2_vbi_flags.h"
#include "xlat/v4l2_sliced_flags.h"

static bool
print_v4l2_clip(struct tcb *tcp, void *elem_buf, size_t elem_size, void *data)
{
	const struct_v4l2_clip *p = elem_buf;
	PRINT_FIELD_OBJ_PTR("{", *p, c, print_v4l2_rect);
	tprints("}");
	return true;
}

#define DECL_print_v4l2_format_fmt(name_)				\
	print_v4l2_format_fmt_ ## name_(struct tcb *const tcp,		\
		const typeof_field(struct_v4l2_format, fmt.name_) *const p)

static bool
DECL_print_v4l2_format_fmt(pix)
{
	PRINT_FIELD_U("{", *p, width);
	PRINT_FIELD_U(", ", *p, height);
	PRINT_FIELD_PIXFMT(", ", *p, pixelformat, v4l2_pix_fmts);
	PRINT_FIELD_XVAL(", ", *p, field, v4l2_fields, "V4L2_FIELD_???");
	PRINT_FIELD_U(", ", *p, bytesperline);
	PRINT_FIELD_U(", ", *p, sizeimage);
	PRINT_FIELD_XVAL(", ", *p, colorspace, v4l2_colorspaces,
			 "V4L2_COLORSPACE_???");
	tprints("}");
	return true;
}

static bool
print_v4l2_plane_pix_format_array_member(struct tcb *tcp, void *elem_buf,
					 size_t elem_size, void *data)
{
	struct_v4l2_plane_pix_format *p = elem_buf;

	PRINT_FIELD_U("{", *p, sizeimage);
	PRINT_FIELD_U(", ", *p, bytesperline);
	tprints("}");

	return true;
}

static bool
DECL_print_v4l2_format_fmt(pix_mp)
{
	PRINT_FIELD_U("{", *p, width);
	PRINT_FIELD_U(", ", *p, height);
	PRINT_FIELD_PIXFMT(", ", *p, pixelformat, v4l2_pix_fmts);
	PRINT_FIELD_XVAL(", ", *p, field, v4l2_fields, "V4L2_FIELD_???");
	PRINT_FIELD_XVAL(", ", *p, colorspace, v4l2_colorspaces,
			 "V4L2_COLORSPACE_???");
	PRINT_FIELD_ARRAY_UPTO(", ", *p, plane_fmt, p->num_planes, tcp,
			       print_v4l2_plane_pix_format_array_member);
	PRINT_FIELD_U(", ", *p, num_planes);
	tprints("}");
	return true;
}

static bool
DECL_print_v4l2_format_fmt(win)
{
	PRINT_FIELD_OBJ_PTR("{", *p, w, print_v4l2_rect);
	PRINT_FIELD_XVAL(", ", *p, field, v4l2_fields, "V4L2_FIELD_???");
	PRINT_FIELD_X(", ", *p, chromakey);

	tprints(", clips=");
	struct_v4l2_clip clip;
	bool rc = print_array(tcp, ptr_to_kulong(p->clips),
			   p->clipcount, &clip, sizeof(clip),
			   tfetch_mem, print_v4l2_clip, 0);

	PRINT_FIELD_U(", ", *p, clipcount);
	PRINT_FIELD_PTR(", ", *p, bitmap);
	if (p->global_alpha)
		PRINT_FIELD_X(", ", *p, global_alpha);
	tprints("}");
	return rc;
}

static bool
DECL_print_v4l2_format_fmt(vbi)
{
	PRINT_FIELD_U("{", *p, sampling_rate);
	PRINT_FIELD_U(", ", *p, offset);
	PRINT_FIELD_U(", ", *p, samples_per_line);
	PRINT_FIELD_PIXFMT(", ", *p, sample_format, v4l2_pix_fmts);
	PRINT_FIELD_D_ARRAY(", ", *p, start);
	PRINT_FIELD_U_ARRAY(", ", *p, count);
	PRINT_FIELD_FLAGS(", ", *p, flags, v4l2_vbi_flags, "V4L2_VBI_???");
	tprints("}");
	return true;
}

static bool
DECL_print_v4l2_format_fmt(sliced)
{
	PRINT_FIELD_FLAGS("{", *p, service_set, v4l2_sliced_flags,
			  "V4L2_SLICED_???");
	PRINT_FIELD_X_ARRAY2D(", ", *p, service_lines);
	PRINT_FIELD_U(", ", *p, io_size);
	tprints("}");
	return true;
}

static bool
DECL_print_v4l2_format_fmt(sdr)
{
	PRINT_FIELD_PIXFMT("{", *p, pixelformat, v4l2_sdr_fmts);
	if (p->buffersize)
		PRINT_FIELD_U(", ", *p, buffersize);
	tprints("}");
	return true;
}

static bool
DECL_print_v4l2_format_fmt(meta)
{
	PRINT_FIELD_PIXFMT("{", *p, dataformat, v4l2_meta_fmts);
	PRINT_FIELD_U(", ", *p, buffersize);
	tprints("}");
	return true;
}

#define PRINT_FIELD_V4L2_FORMAT_FMT(prefix_, where_, fmt_, field_, tcp_, ret_)	\
	do {									\
		STRACE_PRINTF("%s%s.%s=", (prefix_), #fmt_, #field_);		\
		(ret_) = (print_v4l2_format_fmt_ ## field_)			\
				((tcp_), &((where_).fmt_.field_));		\
	} while (0)

static bool
print_v4l2_format_fmt(struct tcb *const tcp, const char *const prefix,
		      const struct_v4l2_format *const f)
{
	bool ret = true;
	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, pix, tcp, ret);
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, pix_mp, tcp, ret);
		break;

	/* OUTPUT_OVERLAY since Linux v2.6.22-rc1~1118^2~179 */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, win, tcp, ret);
		break;

	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, vbi, tcp, ret);
		break;

	/* both since Linux v2.6.14-rc2~64 */
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, sliced, tcp, ret);
		break;

	/* since Linux v4.4-rc1~118^2~14 */
	case V4L2_BUF_TYPE_SDR_OUTPUT:
	/* since Linux v3.15-rc1~85^2~213 */
	case V4L2_BUF_TYPE_SDR_CAPTURE:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, sdr, tcp, ret);
		break;
	/* since Linux v5.0-rc1~181^2~21 */
	case V4L2_BUF_TYPE_META_OUTPUT:
	/* since Linux v4.12-rc1~85^2~71 */
	case V4L2_BUF_TYPE_META_CAPTURE:
		PRINT_FIELD_V4L2_FORMAT_FMT(prefix, *f, fmt, meta, tcp, ret);
		break;
	default:
		return false;
	}
	return ret;
}

static int
print_v4l2_format(struct tcb *const tcp, const kernel_ulong_t arg,
		  const bool is_get)
{
	struct_v4l2_format f;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &f))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", f, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		if (is_get)
			return 0;
		if (!print_v4l2_format_fmt(tcp, ", ", &f)) {
			tprints("}");
			return RVAL_IOCTL_DECODED;
		}

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &f))
		print_v4l2_format_fmt(tcp, is_get ? ", " : "} => {", &f);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_memories.h"

static int
print_v4l2_requestbuffers(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct v4l2_requestbuffers reqbufs;

	if (entering(tcp)) {
		tprints(", ");

		if (umove_or_printaddr(tcp, arg, &reqbufs))
			return RVAL_IOCTL_DECODED;

		PRINT_FIELD_XVAL("{", reqbufs, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		PRINT_FIELD_XVAL(", ", reqbufs, memory, v4l2_memories,
				 "V4L2_MEMORY_???");
		PRINT_FIELD_U(", ", reqbufs, count);

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &reqbufs))
		tprintf(" => %u", reqbufs.count);

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_buf_flags.h"
#include "xlat/v4l2_buf_flags_ts_type.h"
#include "xlat/v4l2_buf_flags_ts_src.h"

#define XLAT_MACROS_ONLY
# include "xlat/v4l2_buf_flags_masks.h"
#undef XLAT_MACROS_ONLY

static void
print_v4l2_buffer_flags(uint32_t val)
{
	const uint32_t ts_type = val & V4L2_BUF_FLAG_TIMESTAMP_MASK;
	const uint32_t ts_src  = val & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	const uint32_t flags   = val & ~ts_type & ~ts_src;

	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW) {
		tprintf("%#" PRIx32, val);
		return;
	}

	if (flags) {
		printflags(v4l2_buf_flags, flags, "V4L2_BUF_FLAG_???");
		tprints("|");
	}
	printxval(v4l2_buf_flags_ts_type, ts_type,
		  "V4L2_BUF_FLAG_TIMESTAMP_???");
	tprints("|");
	printxval(v4l2_buf_flags_ts_src, ts_src,
		  "V4L2_BUF_FLAG_TSTAMP_SRC_???");
}

#define PRINT_FIELD_V4L2_BUFFER_FLAGS(prefix_, where_, field_)	\
	do {							\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);	\
		print_v4l2_buffer_flags((where_).field_);	\
	} while (0)

static void
print_v4l2_timeval(const MPERS_PTR_ARG(kernel_v4l2_timeval_t *) const arg)
{
	const kernel_v4l2_timeval_t *const t = arg;
	kernel_timeval64_t tv;

	if (sizeof(tv.tv_sec) == sizeof(t->tv_sec) &&
	    sizeof(tv.tv_usec) == sizeof(t->tv_usec)) {
		print_timeval64_data_size(t, sizeof(*t));
	} else {
		tv.tv_sec = sign_extend_unsigned_to_ll(t->tv_sec);
		tv.tv_usec = zero_extend_signed_to_ull(t->tv_usec);
		print_timeval64_data_size(&tv, sizeof(tv));
	}
}

#define PRINT_FIELD_V4L2_TIMEVAL(prefix_, where_, field_)	\
	do {							\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);	\
		print_v4l2_timeval(&((where_).field_));		\
	} while (0)

static int
print_v4l2_buffer(struct tcb *const tcp, const unsigned int code,
		  const kernel_ulong_t arg)
{
	kernel_v4l2_buffer_t b;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &b))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", b, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		if (code != VIDIOC_DQBUF)
			PRINT_FIELD_U(", ", b, index);

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &b)) {
		if (code == VIDIOC_DQBUF)
			PRINT_FIELD_U(", ", b, index);
		PRINT_FIELD_XVAL(", ", b, memory, v4l2_memories,
				 "V4L2_MEMORY_???");

		if (b.memory == V4L2_MEMORY_MMAP) {
			PRINT_FIELD_X(", ", b, m.offset);
		} else if (b.memory == V4L2_MEMORY_USERPTR) {
			PRINT_FIELD_PTR(", ", b, m.userptr);
		}

		PRINT_FIELD_U(", ", b, length);
		PRINT_FIELD_U(", ", b, bytesused);
		PRINT_FIELD_V4L2_BUFFER_FLAGS(", ", b, flags);
		if (code == VIDIOC_DQBUF)
			PRINT_FIELD_V4L2_TIMEVAL(", ", b, timestamp);
		tprints(", ...");
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
print_v4l2_framebuffer(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_framebuffer b;

	tprints(", ");
	if (!umove_or_printaddr(tcp, arg, &b)) {
		PRINT_FIELD_X("{", b, capability);
		PRINT_FIELD_X(", ", b, flags);
		PRINT_FIELD_PTR(", ", b, base);
		tprints("}");
	}

	return RVAL_IOCTL_DECODED;
}

static int
print_v4l2_buf_type(struct tcb *const tcp, const kernel_ulong_t arg)
{
	int type;

	tprints(", ");
	if (!umove_or_printaddr(tcp, arg, &type)) {
		tprints("[");
		printxval(v4l2_buf_types, type, "V4L2_BUF_TYPE_???");
		tprints("]");
	}
	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_streaming_capabilities.h"
#include "xlat/v4l2_capture_modes.h"

static void
print_v4l2_streamparm_capture(const struct v4l2_captureparm *const p)
{
	PRINT_FIELD_FLAGS("{", *p, capability, v4l2_streaming_capabilities,
			  "V4L2_CAP_???");
	PRINT_FIELD_FLAGS(", ", *p, capturemode, v4l2_capture_modes,
			  "V4L2_MODE_???");
	PRINT_FIELD_FRACT(", ", *p, timeperframe);
	PRINT_FIELD_X(", ", *p, extendedmode);
	PRINT_FIELD_U(", ", *p, readbuffers);
	tprints("}");
}

static void
print_v4l2_streamparm_output(const struct v4l2_outputparm *const p)
{
	PRINT_FIELD_FLAGS("{", *p, capability, v4l2_streaming_capabilities,
			  "V4L2_CAP_???");
	PRINT_FIELD_FLAGS(", ", *p, outputmode, v4l2_capture_modes,
			  "V4L2_MODE_???");
	PRINT_FIELD_FRACT(", ", *p, timeperframe);
	PRINT_FIELD_X(", ", *p, extendedmode);
	PRINT_FIELD_U(", ", *p, writebuffers);
	tprints("}");
}

#define PRINT_FIELD_V4L2_STREAMPARM_PARM(prefix_, where_, parm_, field_)	\
	do {									\
		STRACE_PRINTF("%s%s.%s=", (prefix_), #parm_, #field_);		\
		print_v4l2_streamparm_ ## field_(&((where_).parm_.field_));	\
	} while (0)

static int
print_v4l2_streamparm(struct tcb *const tcp, const kernel_ulong_t arg,
		      const bool is_get)
{
	struct v4l2_streamparm s;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &s))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", s, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		switch (s.type) {
			case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			case V4L2_BUF_TYPE_VIDEO_OUTPUT:
				if (is_get)
					return 0;
				tprints(", ");
				break;
			default:
				tprints("}");
				return RVAL_IOCTL_DECODED;
		}
	} else {
		if (syserror(tcp) || umove(tcp, arg, &s) < 0) {
			tprints("}");
			return RVAL_IOCTL_DECODED;
		}
		tprints(is_get ? ", " : "} => {");
	}

	if (s.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		PRINT_FIELD_V4L2_STREAMPARM_PARM("", s, parm, capture);
	else
		PRINT_FIELD_V4L2_STREAMPARM_PARM("", s, parm, output);

	if (entering(tcp)) {
		return 0;
	} else {
		tprints("}");
		return RVAL_IOCTL_DECODED;
	}
}

static int
print_v4l2_standard(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_standard s;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &s))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", s, index);

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &s)) {
		PRINT_FIELD_CSTRING(", ", s, name);
		PRINT_FIELD_FRACT(", ", s, frameperiod);
		PRINT_FIELD_U(", ", s, framelines);
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_input_types.h"

static int
print_v4l2_input(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_input i;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &i))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", i, index);

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &i)) {
		PRINT_FIELD_CSTRING(", ", i, name);
		PRINT_FIELD_XVAL(", ", i, type, v4l2_input_types,
				 "V4L2_INPUT_TYPE_???");
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

/*
 * We include it here and not before print_v4l2_ext_controls as we need
 * V4L2_CTRL_CLASS_* definitions for V4L2_CID_*_BASE ones.
 */
#include "xlat/v4l2_control_classes.h"
#include "xlat/v4l2_control_id_bases.h"
#include "xlat/v4l2_control_ids.h"
#include "xlat/v4l2_control_query_flags.h"

static void
print_v4l2_cid(uint32_t cid, bool next_flags)
{
	if (xlat_verbose(xlat_verbosity) == XLAT_STYLE_RAW) {
		tprintf("%#x", cid);
		return;
	}

	if (next_flags) {
		uint32_t flags = cid & v4l2_control_query_flags->flags_mask;

		if (flags) {
			printflags(v4l2_control_query_flags, flags,
				   "V4L2_CTRL_FLAG_NEXT_???");
			tprints("|");

			cid &= ~flags;
		}
	}

	const char *id_name = xlookup(v4l2_control_ids, cid);

	if (id_name) {
		print_xlat_ex(cid, id_name, XLAT_STYLE_DEFAULT);
		return;
	}

	uint64_t class_id = cid;
	const char *class_str = xlookup_le(v4l2_control_classes, &class_id);

	if (!class_str || (cid - class_id) >= 0x10000) {
		print_xlat_ex(cid, "V4L2_CID_???", PXF_DEFAULT_STR);
		return;
	}

	/*
	 * As of now, the longest control class name is V4L2_CTRL_CLASS_IMAGE_SOURCE,
	 * of 28 characters long.
	 */
	char tmp_str[64 + sizeof("+%#") + sizeof(class_id) * 2];

	xsprintf(tmp_str, "%s+%#" PRIx64, class_str, cid - class_id);
	print_xlat_ex(cid, tmp_str, XLAT_STYLE_DEFAULT);
}

#define PRINT_FIELD_V4L2_CID(prefix_, where_, field_, next_)	\
	do {							\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);	\
		print_v4l2_cid((where_).field_, (next_));	\
	} while (0)

static int
print_v4l2_control(struct tcb *const tcp, const kernel_ulong_t arg,
		   const bool is_get)
{
	struct v4l2_control c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;

		PRINT_FIELD_V4L2_CID("{", c, id, false);
		if (!is_get)
			PRINT_FIELD_D(", ", c, value);
		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &c)) {
		if (is_get)
			PRINT_FIELD_D(", ", c, value);
		else
			tprintf(" => %d", c.value);
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_tuner_types.h"
#include "xlat/v4l2_tuner_capabilities.h"
#include "xlat/v4l2_tuner_rxsubchanses.h"
#include "xlat/v4l2_tuner_audmodes.h"

static int
print_v4l2_tuner(struct tcb *const tcp, const kernel_ulong_t arg,
		 const bool is_get)
{
	struct v4l2_tuner c;
	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", c, index);
		if (is_get)
			return 0;
		tprints(", ");
	} else {
		if (syserror(tcp) || umove(tcp, arg, &c) < 0) {
			tprints("}");
			return RVAL_IOCTL_DECODED;
		}
		tprints(is_get ? ", " : "} => {");
	}

	PRINT_FIELD_CSTRING("", c, name);
	PRINT_FIELD_XVAL(", ", c, type, v4l2_tuner_types, "V4L2_TUNER_???");
	PRINT_FIELD_FLAGS(", ", c, capability, v4l2_tuner_capabilities,
			  "V4L2_TUNER_CAP_???");
	PRINT_FIELD_U(", ", c, rangelow);
	PRINT_FIELD_U(", ", c, rangehigh);
	PRINT_FIELD_FLAGS(", ", c, rxsubchans, v4l2_tuner_rxsubchanses,
			  "V4L2_TUNER_SUB_???");
	PRINT_FIELD_XVAL(", ", c, audmode, v4l2_tuner_audmodes,
			 "V4L2_TUNER_MODE_???");
	PRINT_FIELD_D(", ", c, signal);
	PRINT_FIELD_D(", ", c, afc);

	if (entering(tcp)) {
		return 0;
	} else {
		tprints("}");
		return RVAL_IOCTL_DECODED;
	}
}

#include "xlat/v4l2_control_types.h"
#include "xlat/v4l2_control_flags.h"

static int
print_v4l2_queryctrl(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct v4l2_queryctrl c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		set_tcb_priv_ulong(tcp, c.id);
		PRINT_FIELD_V4L2_CID("{", c, id, true);

		return 0;
	}

	/* exiting */
	if (syserror(tcp) || umove(tcp, arg, &c) < 0) {
		tprints("}");
		return RVAL_IOCTL_DECODED;
	}

	unsigned long entry_id = get_tcb_priv_ulong(tcp);

	if (c.id != entry_id) {
		tprints(" => ");
		print_v4l2_cid(c.id, false);
	}

	PRINT_FIELD_XVAL(", ", c, type, v4l2_control_types,
			 "V4L2_CTRL_TYPE_???");
	PRINT_FIELD_CSTRING(", ", c, name);
	if (!abbrev(tcp)) {
		PRINT_FIELD_D(", ", c, minimum);
		PRINT_FIELD_D(", ", c, maximum);
		PRINT_FIELD_D(", ", c, step);
		PRINT_FIELD_D(", ", c, default_value);
		PRINT_FIELD_FLAGS(", ", c, flags, v4l2_control_flags,
				  "V4L2_CTRL_FLAG_???");
		if (!IS_ARRAY_ZERO(c.reserved))
			PRINT_FIELD_ARRAY(", ", c, reserved, tcp,
					  print_xint32_array_member);
	} else {
		tprints(", ...");
	}
	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
print_v4l2_query_ext_ctrl(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_query_ext_ctrl c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		set_tcb_priv_ulong(tcp, c.id);
		PRINT_FIELD_V4L2_CID("{", c, id, true);

		return 0;
	}

	/* exiting */
	if (syserror(tcp) || umove(tcp, arg, &c) < 0) {
		tprints("}");
		return RVAL_IOCTL_DECODED;
	}

	unsigned long entry_id = get_tcb_priv_ulong(tcp);

	if (c.id != entry_id) {
		tprints(" => ");
		print_v4l2_cid(c.id, false);
	}

	PRINT_FIELD_XVAL(", ", c, type, v4l2_control_types,
			 "V4L2_CTRL_TYPE_???");
	PRINT_FIELD_CSTRING(", ", c, name);
	if (!abbrev(tcp)) {
		PRINT_FIELD_D(", ", c, minimum);
		PRINT_FIELD_D(", ", c, maximum);
		PRINT_FIELD_U(", ", c, step);
		PRINT_FIELD_D(", ", c, default_value);
		PRINT_FIELD_FLAGS(", ", c, flags, v4l2_control_flags,
				  "V4L2_CTRL_FLAG_???");
		PRINT_FIELD_U(", ", c, elem_size);
		PRINT_FIELD_U(", ", c, elems);
		PRINT_FIELD_U(", ", c, nr_of_dims);
		PRINT_FIELD_ARRAY_UPTO(", ", c, dims, c.nr_of_dims, tcp,
				       print_uint32_array_member);
		if (!IS_ARRAY_ZERO(c.reserved))
			PRINT_FIELD_ARRAY(", ", c, reserved, tcp,
					  print_xint32_array_member);
	} else {
		tprints(", ...");
	}
	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
print_v4l2_cropcap(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct v4l2_cropcap c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", c, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");

		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &c)) {
		PRINT_FIELD_OBJ_PTR(", ", c, bounds, print_v4l2_rect);
		PRINT_FIELD_OBJ_PTR(", ", c, defrect, print_v4l2_rect);
		PRINT_FIELD_FRACT(", ", c, pixelaspect);
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static int
print_v4l2_crop(struct tcb *const tcp, const kernel_ulong_t arg,
		const bool is_get)
{
	struct v4l2_crop c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", c, type, v4l2_buf_types,
				 "V4L2_BUF_TYPE_???");
		if (is_get)
			return 0;
		PRINT_FIELD_OBJ_PTR(", " , c, c, print_v4l2_rect);
	} else {
		if (!syserror(tcp) && !umove(tcp, arg, &c))
			PRINT_FIELD_OBJ_PTR(", " , c, c, print_v4l2_rect);
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static bool
print_v4l2_ext_control(struct tcb *tcp, void *elem_buf, size_t elem_size, void *data)
{
	const struct_v4l2_ext_control *p = elem_buf;

	PRINT_FIELD_XVAL("{", *p, id, v4l2_control_ids, "V4L2_CID_???");
	PRINT_FIELD_U(", ", *p, size);
	if (p->size > 0) {
		tprints(", string=");
		printstrn(tcp, ptr_to_kulong(p->string), p->size);
	} else {
		PRINT_FIELD_D(", ", *p, value);
		PRINT_FIELD_D(", ", *p, value64);
	}
	tprints("}");

	return true;
}

static int
print_v4l2_ext_controls(struct tcb *const tcp, const kernel_ulong_t arg,
			const bool is_get)
{
	struct_v4l2_ext_controls c;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &c))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_XVAL("{", c, ctrl_class, v4l2_control_classes,
				 "V4L2_CTRL_CLASS_???");
		PRINT_FIELD_U(", ", c, count);
		if (!c.count) {
			tprints("}");
			return RVAL_IOCTL_DECODED;
		}
		if (is_get)
			return 0;
		tprints(", ");
	} else {
		if (umove(tcp, arg, &c) < 0) {
			tprints("}");
			return RVAL_IOCTL_DECODED;
		}
		tprints(is_get ? ", " : "} => {");
	}

	tprints("controls=");
	struct_v4l2_ext_control ctrl;
	bool fail = !print_array(tcp, ptr_to_kulong(c.controls), c.count,
				 &ctrl, sizeof(ctrl),
				 tfetch_mem_ignore_syserror,
				 print_v4l2_ext_control, 0);

	if (exiting(tcp) && syserror(tcp))
		PRINT_FIELD_U(", ", c, error_idx);

	if (exiting(tcp) || fail) {
		tprints("}");
		return RVAL_IOCTL_DECODED;
	}

	/* entering */
	return 0;
}

#include "xlat/v4l2_framesize_types.h"

static void
print_v4l2_frmsize_discrete(const MPERS_PTR_ARG(struct_v4l2_frmsize_discrete *) const arg)
{
	const struct_v4l2_frmsize_discrete *const p = arg;
	PRINT_FIELD_U("{", *p, width);
	PRINT_FIELD_U(", ", *p, height);
	tprints("}");
}

static void
print_v4l2_frmsize_stepwise(const MPERS_PTR_ARG(struct_v4l2_frmsize_stepwise *) const arg)
{
	const struct_v4l2_frmsize_stepwise *const p = arg;
	PRINT_FIELD_U("{", *p, min_width);
	PRINT_FIELD_U(", ", *p, max_width);
	PRINT_FIELD_U(", ", *p, step_width);
	PRINT_FIELD_U(", ", *p, min_height);
	PRINT_FIELD_U(", ", *p, max_height);
	PRINT_FIELD_U(", ", *p, step_height);
	tprints("}");
}

#define PRINT_FIELD_V4L2_FRMSIZE_TYPE(prefix_, where_, field_)		\
	do {								\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);		\
		print_v4l2_frmsize_ ## field_(&((where_).field_));	\
	} while (0)

static int
print_v4l2_frmsizeenum(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_frmsizeenum s;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &s))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", s, index);
		PRINT_FIELD_PIXFMT(", ", s, pixel_format, v4l2_pix_fmts);
		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &s)) {
		PRINT_FIELD_XVAL(", ", s, type, v4l2_framesize_types,
				 "V4L2_FRMSIZE_TYPE_???");
		switch (s.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			PRINT_FIELD_V4L2_FRMSIZE_TYPE(", ", s, discrete);
			break;
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			PRINT_FIELD_V4L2_FRMSIZE_TYPE(", ", s, stepwise);
			break;
		}
	}
	tprints("}");
	return RVAL_IOCTL_DECODED;
}

#include "xlat/v4l2_frameinterval_types.h"

static void
print_v4l2_frmival_stepwise(const MPERS_PTR_ARG(struct_v4l2_frmival_stepwise *) const arg)
{
	const struct_v4l2_frmival_stepwise *const p = arg;
	PRINT_FIELD_FRACT("{", *p, min);
	PRINT_FIELD_FRACT(", ", *p, max);
	PRINT_FIELD_FRACT(", ", *p, step);
	tprints("}");
}

#define PRINT_FIELD_V4L2_FRMIVAL_STEPWISE(prefix_, where_, field_)	\
	do {								\
		STRACE_PRINTF("%s%s=", (prefix_), #field_);		\
		print_v4l2_frmival_stepwise(&((where_).field_));	\
	} while (0)

static int
print_v4l2_frmivalenum(struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_v4l2_frmivalenum f;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &f))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", f, index);
		PRINT_FIELD_PIXFMT(", ", f, pixel_format, v4l2_pix_fmts);
		PRINT_FIELD_U(", ", f, width);
		PRINT_FIELD_U(", ", f, height);
		return 0;
	}

	if (!syserror(tcp) && !umove(tcp, arg, &f)) {
		PRINT_FIELD_XVAL(", ", f, type, v4l2_frameinterval_types,
				 "V4L2_FRMIVAL_TYPE_???");
		switch (f.type) {
		case V4L2_FRMIVAL_TYPE_DISCRETE:
			PRINT_FIELD_FRACT(", ", f, discrete);
			break;
		case V4L2_FRMIVAL_TYPE_STEPWISE:
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			PRINT_FIELD_V4L2_FRMIVAL_STEPWISE(", ", f, stepwise);
			break;
		}
	}

	tprints("}");

	return RVAL_IOCTL_DECODED;
}

static void
print_v4l2_create_buffers_format(const typeof_field(struct_v4l2_create_buffers, format) *const p,
				 struct tcb *const tcp)
{
	PRINT_FIELD_XVAL("{", *p, type, v4l2_buf_types,
			 "V4L2_BUF_TYPE_???");
	print_v4l2_format_fmt(tcp, ", ", (const struct_v4l2_format *) p);
	tprints("}");
}

static int
print_v4l2_create_buffers(struct tcb *const tcp, const kernel_ulong_t arg)
{
	static const char fmt[] = "{index=%u, count=%u}";
	static char outstr[sizeof(fmt) + sizeof(int) * 6];

	struct_v4l2_create_buffers b;

	if (entering(tcp)) {
		tprints(", ");
		if (umove_or_printaddr(tcp, arg, &b))
			return RVAL_IOCTL_DECODED;
		PRINT_FIELD_U("{", b, count);
		PRINT_FIELD_XVAL(", ", b, memory, v4l2_memories,
				 "V4L2_MEMORY_???");
		PRINT_FIELD_OBJ_PTR(", ", b, format,
				    print_v4l2_create_buffers_format, tcp);
		tprints("}");
		return 0;
	}

	if (syserror(tcp) || umove(tcp, arg, &b))
		return RVAL_IOCTL_DECODED;

	xsprintf(outstr, fmt, b.index, b.count);
	tcp->auxstr = outstr;

	return RVAL_IOCTL_DECODED | RVAL_STR;
}

MPERS_PRINTER_DECL(int, v4l2_ioctl, struct tcb *const tcp,
		   const unsigned int code, const kernel_ulong_t arg)
{
	if (!verbose(tcp))
		return RVAL_DECODED;

	switch (code) {
	case VIDIOC_QUERYCAP: /* R */
		return print_v4l2_capability(tcp, arg);

	case VIDIOC_ENUM_FMT: /* RW */
		return print_v4l2_fmtdesc(tcp, arg);

	case VIDIOC_G_FMT: /* RW */
	case VIDIOC_S_FMT: /* RW */
	case VIDIOC_TRY_FMT: /* RW */
		return print_v4l2_format(tcp, arg, code == VIDIOC_G_FMT);

	case VIDIOC_REQBUFS: /* RW */
		return print_v4l2_requestbuffers(tcp, arg);

	case VIDIOC_QUERYBUF: /* RW */
	case VIDIOC_QBUF: /* RW */
	case VIDIOC_DQBUF: /* RW */
		return print_v4l2_buffer(tcp, code, arg);

	case VIDIOC_G_FBUF: /* R */
		if (entering(tcp))
			return 0;
		ATTRIBUTE_FALLTHROUGH;
	case VIDIOC_S_FBUF: /* W */
		return print_v4l2_framebuffer(tcp, arg);

	case VIDIOC_STREAMON: /* W */
	case VIDIOC_STREAMOFF: /* W */
		return print_v4l2_buf_type(tcp, arg);

	case VIDIOC_G_PARM: /* RW */
	case VIDIOC_S_PARM: /* RW */
		return print_v4l2_streamparm(tcp, arg, code == VIDIOC_G_PARM);

	case VIDIOC_G_STD: /* R */
		if (entering(tcp))
			return 0;
		ATTRIBUTE_FALLTHROUGH;
	case VIDIOC_S_STD: /* W */
		tprints(", ");
		printnum_int64(tcp, arg, "%#" PRIx64);
		break;

	case VIDIOC_ENUMSTD: /* RW */
		return print_v4l2_standard(tcp, arg);

	case VIDIOC_ENUMINPUT: /* RW */
		return print_v4l2_input(tcp, arg);

	case VIDIOC_G_CTRL: /* RW */
	case VIDIOC_S_CTRL: /* RW */
		return print_v4l2_control(tcp, arg, code == VIDIOC_G_CTRL);

	case VIDIOC_G_TUNER: /* RW */
	case VIDIOC_S_TUNER: /* RW */
		return print_v4l2_tuner(tcp, arg, code == VIDIOC_G_TUNER);

	case VIDIOC_QUERYCTRL: /* RW */
		return print_v4l2_queryctrl(tcp, arg);

	case VIDIOC_QUERY_EXT_CTRL: /* RW */
		return print_v4l2_query_ext_ctrl(tcp, arg);

	case VIDIOC_G_INPUT: /* R */
		if (entering(tcp))
			return 0;
		ATTRIBUTE_FALLTHROUGH;
	case VIDIOC_S_INPUT: /* RW */
		tprints(", ");
		printnum_int(tcp, arg, "%u");
		break;

	case VIDIOC_CROPCAP: /* RW */
		return print_v4l2_cropcap(tcp, arg);

	case VIDIOC_G_CROP: /* RW */
	case VIDIOC_S_CROP: /* W */
		return print_v4l2_crop(tcp, arg, code == VIDIOC_G_CROP);

	case VIDIOC_S_EXT_CTRLS: /* RW */
	case VIDIOC_TRY_EXT_CTRLS: /* RW */
	case VIDIOC_G_EXT_CTRLS: /* RW */
		return print_v4l2_ext_controls(tcp, arg,
					       code == VIDIOC_G_EXT_CTRLS);

	case VIDIOC_ENUM_FRAMESIZES: /* RW */
		return print_v4l2_frmsizeenum(tcp, arg);

	case VIDIOC_ENUM_FRAMEINTERVALS: /* RW */
		return print_v4l2_frmivalenum(tcp, arg);

	case VIDIOC_CREATE_BUFS: /* RW */
		return print_v4l2_create_buffers(tcp, arg);

	default:
		return RVAL_DECODED;
	}

	return RVAL_IOCTL_DECODED;
}
