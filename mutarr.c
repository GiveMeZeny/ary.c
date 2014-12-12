#include "mutarr.h"

static void *ma_xrealloc_builtin(void *ptr, size_t nmemb, size_t size)
{
	return realloc(ptr, nmemb * size);
}

ma_xalloc ma_xrealloc = ma_xrealloc_builtin;
ma_xdealloc ma_xfree = free;

void ma_setrealloc(ma_xalloc routine)
{
	ma_xrealloc = routine;
}

void ma_setfree(ma_xdealloc routine)
{
	ma_xfree = routine;
}

int (ma_swap)(struct mutarrb *a, struct mutarrb *b)
{
	size_t alloc, len;
	void *buf;

	if (a->sz != b->sz)
		return 0;
	alloc = a->alloc;
	len = a->len;
	buf = a->buf;
	a->alloc = b->alloc;
	a->len = b->len;
	a->buf = b->buf;
	b->alloc = alloc;
	b->len = len;
	b->buf = buf;
	return 1;
}

void *(ma_fullswap)(struct mutarrb *a, struct mutarrb *b)
{
	void *tmp;

	if (a->sz != b->sz)
		return NULL;
	tmp = ma_xrealloc(NULL, 1, (sizeof(*a) > a->sz) ? sizeof(*a) : a->sz);
	if (!tmp)
		return NULL;
	memcpy(tmp, a, sizeof(*a));
	memcpy(a, b, sizeof(*a));
	memcpy(b, tmp, sizeof(*a));
	return tmp;
}

void *(ma_splice)(struct mutarrb *ma, int offset, size_t len, const void *data,
		size_t dlen)
{
	size_t pos = ma_i2pos(offset, ma->len);
	char *buf = (char *)ma->buf + (pos * ma->sz);

	if (len > ma->len - pos)
		len = ma->len - pos;
	if (len && ma->dtor) {
		ma_callb dtor = ma->dtor;
		char *elem = buf;
		void *userp = ma->userp;
		size_t i;

		for (i = len; i--; elem += ma->sz)
			dtor(elem, userp);
	}
	if (len != dlen) {
		if (len < dlen && !(ma_grow)(ma, dlen - len))
			return NULL;
		buf = (char *)ma->buf + (pos * ma->sz);
		memmove(buf + (dlen * ma->sz), buf + (len * ma->sz),
			(ma->len - pos - len) * ma->sz);
	}
	memcpy(buf, data, dlen * ma->sz);
	ma->len = ma->len - len + dlen;
	return buf;
}

int (ma_insertsorted)(struct mutarrb *ma, const void *data)
{
	size_t l, h, m;
	char *buf;

	if (!ma->cmp || (ma->len == ma->alloc && !(ma_grow)(ma, 1)))
		return 0;
	buf = ma->buf;
	for (l = 0, h = ma->len; l < h;) {
		ma_cmpcb cmp = ma->cmp;

		m = (l + h) / 2;
		if (cmp(buf + (m * ma->sz), data) > 0)
			h = m;
		else
			l = m + 1;
	}
	buf += (l * ma->sz);
	memmove(buf + ma->sz, buf, (ma->len - l) * ma->sz);
	memcpy(buf, data, ma->sz);
	ma->len++;
	return 1;
}

int (ma_copy)(struct mutarrb *ma, int offset, const void *data, size_t dlen,
	      ma_copycb copy)
{
	const char *from, *end;
	char *to;
	size_t retval = 0, i;

	to = (ma_insertmanyp)(ma, offset, dlen);
	if (!to)
		return -1;
	end = (char *)ma->buf + (ma->len * ma->sz);
	from = data;
	for (i = dlen; i--; from += ma->sz) {
		if (copy(to, from)) {
			retval++;
			to += ma->sz;
		}
	}
	memmove(to, to + ((dlen - retval) * ma->sz),
		end - to + ((dlen - retval) * ma->sz));
	ma->len -= dlen - retval;
	return (int)retval;
}

int (ma_index)(struct mutarrb *ma, int start, const void *data)
{
	size_t pos = ma_i2pos(start, ma->len), i;
	char *elem = (char *)ma->buf + (pos * ma->sz);

	if (ma->cmp) {
		ma_cmpcb cmp = ma->cmp;

		for (i = pos; i < ma->len; i++, elem += ma->sz) {
			if (!cmp(elem, data))
				return (int)i;
		}
	} else {
		for (i = pos; i < ma->len; i++, elem += ma->sz) {
			if (!memcmp(elem, data, ma->sz))
				return (int)i;
		}
	}
	return -1;
}

int (ma_rindex)(struct mutarrb *ma, int start, const void *data)
{
	size_t pos = ma_i2pos(start, ma->len), i;
	char *elem = (char *)ma->buf + (pos * ma->sz);
	int save = -1;

	if (ma->cmp) {
		ma_cmpcb cmp = ma->cmp;

		for (i = pos; i < ma->len; i++, elem += ma->sz) {
			if (!cmp(elem, data))
				save = (int)i;
		}
	}
	else {
		for (i = pos; i < ma->len; i++, elem += ma->sz) {
			if (!memcmp(elem, data, ma->sz))
				save = (int)i;
		}
	}
	return save;
}

int (ma_search)(struct mutarrb *ma, int start, const void *data)
{
	size_t pos;
	char *found;

	if (!ma->cmp)
		return -1;
	pos = ma_i2pos(start, ma->len);
	found = bsearch(data, (char *)ma->buf + (pos * ma->sz), ma->len - pos,
			ma->sz, ma->cmp);
	if (!found)
		return -1;
	return (found - (char *)ma->buf) / ma->sz;
}

int (ma_xchg)(struct mutarrb *ma, int a, int b)
{
	size_t pos1, pos2;
	void *p, *q, *tmp;

	tmp = ma_xrealloc(NULL, 1, ma->sz);
	if (!tmp)
		return 0;
	pos1 = ma_i2pos(a, ma->len);
	pos2 = ma_i2pos(b, ma->len);
	p = (char *)ma->buf + (pos1 * ma->sz);
	q = (char *)ma->buf + (pos2 * ma->sz);
	memcpy(tmp, p, ma->sz);
	memcpy(p, q, ma->sz);
	memcpy(q, tmp, ma->sz);
	ma_xfree(tmp);
	return 1;
}

int (ma_reverse)(struct mutarrb *ma)
{
	size_t i, j;
	char *p, *q, *tmp;

	tmp = ma_xrealloc(NULL, 1, ma->sz);
	if (!tmp)
		return 0;
	j = ma->len - 1;
	p = (char *)ma->buf;
	q = p + (j * ma->sz);
	for (i = 0; i < (ma->len / 2); i++, j--) {
		memcpy(tmp, p, ma->sz);
		memcpy(p, q, ma->sz);
		memcpy(q, tmp, ma->sz);
		p += ma->sz;
		q -= ma->sz;
	}
	ma_xfree(tmp);
	return 1;
}

int (ma_join)(struct mutarrb *ma, char **ret, const char *sep)
{
	struct mutarr(char) strbuf;
	ma_tostrcb tostr = ma->tostr;
	char *elem = (char *)ma->buf, *tmp;
	size_t seplen = sep ? strlen(sep) : 0, i, len;
	void *tmpret;
	int tmplen;

	ma_init(&strbuf, 0);
	for (i = 0; i < ma->len; i++) {
		if (tostr) {
			tmplen = tostr(&tmp, elem);
			if (tmplen > 0) {
				tmpret = ma_pushmany(&strbuf, tmp, tmplen);
				ma_xfree(tmp);
				if (!tmpret)
					goto error;
			} else if (!tmplen)
				ma_xfree(tmp);
		} else {
			tmp = *(char **)elem;
			if (!ma_pushmany(&strbuf, tmp, strlen(tmp)))
				goto error;
		}
		if (seplen)
			if (!ma_pushmany(&strbuf, sep, seplen))
				goto error;
		elem += ma->sz;
	}
	if (!strbuf.len) {
		if (!(*ret = strdup("")))
			goto error;
		return 0;
	}
	ma_setlen(&strbuf, strbuf.len - seplen);
	if (!ma_push(&strbuf, '\0'))
		goto error;
	ma_shrinktofit(&strbuf);
	*ret = ma_detach(&strbuf, &len);
	return (int)len;

error:
	ma_release(&strbuf);
	*ret = NULL;
	return -1;
}
