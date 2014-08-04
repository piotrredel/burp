#include <yajl/yajl_parse.h>

#include "include.h"

static int map_depth=0;

static unsigned long number=0;
static struct cstat *cnew=NULL;
static struct cstat *current=NULL;
static struct cstat **cslist=NULL;
static char lastkey[32]="";

static int input_integer(void *ctx, long long val)
{
	if(!strcmp(lastkey, "number"))
	{
		if(!current) goto error;
		number=(unsigned long)val;
		return 1;
	}
	else if(!strcmp(lastkey, "timestamp"))
	{
		time_t t;
		if(!current) goto error;
		bu_list_free(&current->bu);
		if(!(current->bu=bu_alloc()))
			return 0;
		t=(unsigned long)val;
		if(!(current->bu->timestamp=strdup_w(
			getdatestr(t), __func__))) return 0;
		current->bu->bno=number;
		number=0;
		return 1;
	}
error:
	logp("Unexpected integer: %s %llu\n", lastkey, val);
        return 0;
}

static int input_string(void *ctx, const unsigned char *val, size_t len)
{
	char *str;
	if(!(str=(char *)malloc_w(len+2, __func__)))
		return 0;
	snprintf(str, len+1, "%s", val);

	if(!strcmp(lastkey, "name"))
	{
		if(cnew) goto error;
		if(!(current=cstat_get_by_name(*cslist, str)))
		{
			if(!(cnew=cstat_alloc())
			  || cstat_init(cnew, str, NULL))
				goto error;
			current=cnew;
		}
		goto end;
	}
	else if(!strcmp(lastkey, "status"))
	{
		if(!current) goto error;
		current->status=cstat_str_to_status(str);
		goto end;
	}
error:
	logp("Unexpected string: %s %s\n", lastkey, str);
	free_w(&str);
        return 0;
end:
	free_w(&str);
	return 1;
}

static int input_map_key(void *ctx, const unsigned char *val, size_t len)
{
        snprintf(lastkey, len+1, "%s", val);
        logp("mapkey: %s\n", lastkey);
        return 1;
}

static int input_start_map(void *ctx)
{
        logp("startmap\n");
	map_depth++;
        return 1;
}

static int input_end_map(void *ctx)
{
        logp("endmap\n");
	map_depth--;
        return 1;
}

static int input_start_array(void *ctx)
{
        logp("start arr\n");
        return 1;
}

static int input_end_array(void *ctx)
{
	if(!strcmp(lastkey, "backups"))
	{
		if(cnew && cnew->bu)
		{
			if(cstat_add_to_list(cslist, cnew)) return -1;
			current=NULL;
			cnew=NULL;
		}
	}
        return 1;
}

static yajl_callbacks callbacks = {
        NULL,
        NULL,
        input_integer,
        NULL,
        NULL,
        input_string,
        input_start_map,
        input_map_key,
        input_end_map,
        input_start_array,
        input_end_array
};

static void do_yajl_error(yajl_handle yajl, struct asfd *asfd)
{
	unsigned char *str;
	str=yajl_get_error(yajl, 1,
		(const unsigned char *)asfd->rbuf->buf, asfd->rbuf->len);
	logp("yajl error: %s\n", (const char *)str);
	yajl_free_error(yajl, str);
}

// Client records will be coming through in alphabetical order.
// FIX THIS: If a client is deleted on the server, it is not deleted from
// clist.
int json_input(struct asfd *asfd, struct cstat **clist)
{
        static yajl_handle yajl=NULL;
	cslist=clist;

	if(!yajl)
	{
		if(!(yajl=yajl_alloc(&callbacks, NULL, NULL)))
			goto error;
		yajl_config(yajl, yajl_dont_validate_strings, 1);
	}
	if(yajl_parse(yajl, (const unsigned char *)asfd->rbuf->buf,
		asfd->rbuf->len)!=yajl_status_ok)
	{
		do_yajl_error(yajl, asfd);
		goto error;
	}

	if(!map_depth)
	{
		// Got to the end of the JSON object.
		if(yajl_complete_parse(yajl)!=yajl_status_ok)
		{
			do_yajl_error(yajl, asfd);
			goto error;
		}
		yajl_free(yajl);
		yajl=NULL;
	}

	return 0;
error:
	yajl_free(yajl);
	yajl=NULL;
	return -1;
}
