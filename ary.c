#include "ary.h"

/* taken from OpenBSD */
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

static void *ary_xrealloc_builtin(void *ptr, size_t nmemb, size_t size)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb > 0 && SIZE_MAX / nmemb < size)
		return NULL;
	return realloc(ptr, nmemb * size);
}

ary_xalloc_t ary_xrealloc = ary_xrealloc_builtin;
static ary_xdealloc_t ary_xfree = free;

void ary_cb_freevoidptr(void *buf, void *userp)
{
	(void)userp;
	ary_xfree(*(void **)buf);
}

void ary_cb_freecharptr(void *buf, void *userp)
{
	(void)userp;
	ary_xfree(*(char **)buf);
}

int ary_cb_cmpint(const void *a, const void *b)
{
	int x = *(const int *)a, y = *(const int *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_cmplong(const void *a, const void *b)
{
	long x = *(const long *)a, y = *(const long *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_cmpvlong(const void *a, const void *b)
{
	long long x = *(const long long *)a, y = *(const long long *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_cmpsize_t(const void *a, const void *b)
{
	size_t x = *(const size_t *)a, y = *(const size_t *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_cmpdouble(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_cmpchar(const void *a, const void *b)
{
	char x = *(const char *)a, y = *(const char *)b;

	return x > y ? 1 : x < y ? -1 : 0;
}

int ary_cb_strcmp(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

int ary_cb_strcasecmp(const void *a, const void *b)
{
	return strcasecmp(*(char **)a, *(char **)b);
}

static const size_t snprintf_bufsize = 32;

int ary_cb_voidptrtostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%p", *(void **)elem);
}

int ary_cb_inttostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%d", *(int *)elem);
}

int ary_cb_longtostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%ld", *(long *)elem);
}

int ary_cb_vlongtostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%lld", *(long long *)elem);
}

int ary_cb_size_ttostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%zu", *(size_t *)elem);
}

int ary_cb_doubletostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%g", *(double *)elem);
}

int ary_cb_chartostr(char **ret, const void *elem)
{
	if (!(*ret = ary_xrealloc(NULL, snprintf_bufsize, 1)))
		return -1;
	return snprintf(*ret, snprintf_bufsize, "%d", *(char *)elem);
}

void ary_use_as_realloc(ary_xalloc_t routine)
{
	ary_xrealloc = routine;
}

void ary_use_as_free(ary_xdealloc_t routine)
{
	ary_xfree = routine;
}

void ary_freebuf(struct aryb *ary)
{
	if (ary->len && ary->dtor) {
		ary_elemcb_t dtor = ary->dtor;
		char *elem = ary->buf;
		void *userp = ary->userp;
		size_t i;

		for (i = ary->len; i--; elem += ary->sz)
			dtor(elem, userp);
	}
	ary_xfree(ary->buf);
}

void *(ary_detach)(struct aryb *ary, size_t *ret)
{
	void *buf;

	(ary_shrinktofit)(ary);
	buf = ary->buf;
	if (ret)
		*ret = ary->len;
	ary->alloc = ary->len = 0;
	ary->buf = NULL;
	return buf;
}

int (ary_shrinktofit)(struct aryb *ary)
{
	void *buf;

	if (ary->alloc == ary->len)
		return 1;
	if (ary->len) {
		buf = ary_xrealloc(ary->buf, ary->len, ary->sz);
		if (!buf)
			return 0;
	} else {
		ary_xfree(ary->buf);
		buf = NULL;
	}
	ary->alloc = ary->len;
	ary->buf = buf;
	return 1;
}

void *(ary_splicep)(struct aryb *ary, size_t pos, size_t rlen, size_t alen)
{
	char *buf;

	if (pos > ary->len)
		pos = ary->len;
	if (rlen > ary->len - pos)
		rlen = ary->len - pos;
	if (alen > rlen && !(ary_grow)(ary, alen - rlen))
		return NULL;
	buf = (char *)ary->buf + (pos * ary->sz);
	if (rlen && ary->dtor) {
		ary_elemcb_t dtor = ary->dtor;
		char *elem = buf;
		void *userp = ary->userp;
		size_t i;

		for (i = rlen; i--; elem += ary->sz)
			dtor(elem, userp);
	}
	if (rlen != alen && pos < ary->len)
		memmove(buf + (alen * ary->sz), buf + (rlen * ary->sz),
		        (ary->len - pos - rlen) * ary->sz);
	ary->len = ary->len - rlen + alen;
	return buf;
}

int (ary_index)(struct aryb *ary, size_t *ret, size_t start, const void *data,
                ary_cmpcb_t comp)
{
	size_t i;
	char *elem = (char *)ary->buf + (start * ary->sz);

	for (i = start; i < ary->len; i++, elem += ary->sz) {
		if (comp ? !comp(elem, data) : !memcmp(elem, data, ary->sz)) {
			if (ret)
				*ret = i;
			return 1;
		}
	}
	return 0;
}

int (ary_rindex)(struct aryb *ary, size_t *ret, size_t start, const void *data,
                 ary_cmpcb_t comp)
{
	size_t i;
	char *elem;

	if (start >= ary->len)
		start = ary->len - 1;
	elem = (char *)ary->buf + (start * ary->sz);
	for (i = start; i--; elem -= ary->sz) {
		if (comp ? !comp(elem, data) : !memcmp(elem, data, ary->sz)) {
			if (ret)
				*ret = i;
			return 1;
		}
	}
	return 0;
}

int (ary_reverse)(struct aryb *ary)
{
	size_t i, j;
	char *p, *q, *tmp;

	tmp = ary_xrealloc(NULL, 1, ary->sz);
	if (!tmp)
		return 0;
	j = ary->len - 1;
	p = (char *)ary->buf;
	q = p + (j * ary->sz);
	for (i = 0; i < ary->len / 2; i++, j--) {
		memcpy(tmp, p, ary->sz);
		memcpy(p, q, ary->sz);
		memcpy(q, tmp, ary->sz);
		p += ary->sz;
		q -= ary->sz;
	}
	ary_xfree(tmp);
	return 1;
}

int (ary_join)(struct aryb *ary, char **ret, const char *sep,
               ary_joincb_t stringify)
{
	struct ary_char strbuf;
	char *elem = (char *)ary->buf, *tmp;
	size_t seplen = sep ? strlen(sep) : 0, i, len;
	int tmplen, tmpret;

	if (!ary_init(&strbuf, 1024))
		goto error;
	for (i = 0; i < ary->len; i++, elem += ary->sz) {
		if (stringify) {
			tmplen = stringify(&tmp, elem);
			if (tmplen > 0) {
				tmpret = ary_splice(&strbuf, strbuf.len, 0,
				                    tmp, tmplen);
				ary_xfree(tmp);
				if (!tmpret)
					goto error;
			} else if (!tmplen) {
				ary_xfree(tmp);
			}
		} else {
			tmp = *(char **)elem;
			if (!tmp)
				continue;
			if (!ary_splice(&strbuf, strbuf.len, 0, tmp,
			                strlen(tmp)))
				goto error;
		}
		if (seplen) {
			if (!ary_splice(&strbuf, strbuf.len, 0, sep, seplen))
				goto error;
		}
	}
	if (!strbuf.len) {
		if (!(*ret = strdup("")))
			goto error;
		return 0;
	}
	ary_setlen(&strbuf, strbuf.len - seplen + 1);
	strbuf.buf[strbuf.len - 1] = '\0';
	ary_shrinktofit(&strbuf);
	*ret = ary_detach(&strbuf, &len);
	return (int)len;

error:
	ary_release(&strbuf);
	*ret = NULL;
	return -1;
}

int (ary_swap)(struct aryb *ary, size_t a, size_t b)
{
	char *p, *q, *tmp;

	if (a >= ary->len)
		a = ary->len - 1;
	if (b >= ary->len)
		b = ary->len - 1;
	if (a == b)
		return 1;
	tmp = ary_xrealloc(NULL, 1, ary->sz);
	if (!tmp)
		return 0;
	p = (char *)ary->buf + (a * ary->sz);
	q = (char *)ary->buf + (b * ary->sz);
	memcpy(tmp, p, ary->sz);
	memcpy(p, q, ary->sz);
	memcpy(q, tmp, ary->sz);
	ary_xfree(tmp);
	return 1;
}

int (ary_search)(struct aryb *ary, size_t *ret, size_t start, const void *data,
                 ary_cmpcb_t comp)
{
	size_t count = (start < ary->len) ? ary->len - start : 0;
	char *elem = (char *)ary->buf + (start * ary->sz);
	void *ptr = bsearch(data, elem, count, ary->sz, comp);

	if (!ptr)
		return 0;
	if (ret)
		*ret = (size_t)((uintptr_t)ptr - (uintptr_t)ary->buf);
	return 1;
}
