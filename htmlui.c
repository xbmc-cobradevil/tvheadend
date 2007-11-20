/*
 *  tvheadend, HTML user interface
 *  Copyright (C) 2007 Andreas �man
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "tvhead.h"
#include "htmlui.h"
#include "channels.h"
#include "epg.h"
#include "pvr.h"
#include "strtab.h"

static struct strtab recstatustxt[] = {
  { "Scheduled",          HTSTV_PVR_STATUS_SCHEDULED      },
  { "Recording",          HTSTV_PVR_STATUS_RECORDING      },
  { "Done",               HTSTV_PVR_STATUS_DONE           },

  { "Recording aborted",  HTSTV_PVR_STATUS_ABORTED        },

  { "No transponder",     HTSTV_PVR_STATUS_NO_TRANSPONDER },
  { "File error",         HTSTV_PVR_STATUS_FILE_ERROR     },
  { "Disk full",          HTSTV_PVR_STATUS_DISK_FULL      },
  { "Buffer error",       HTSTV_PVR_STATUS_BUFFER_ERROR   },
};

static struct strtab recstatuscolor[] = {
  { "#3333aa",      HTSTV_PVR_STATUS_SCHEDULED      },
  { "#aa3333",      HTSTV_PVR_STATUS_RECORDING      },
  { "#33aa33",      HTSTV_PVR_STATUS_DONE           },
  { "#aa3333",      HTSTV_PVR_STATUS_ABORTED        },
  { "#aa3333",      HTSTV_PVR_STATUS_NO_TRANSPONDER },
  { "#aa3333",      HTSTV_PVR_STATUS_FILE_ERROR     },
  { "#aa3333",      HTSTV_PVR_STATUS_DISK_FULL      },
  { "#aa3333",      HTSTV_PVR_STATUS_BUFFER_ERROR   },
};


static int
pvrstatus_to_html(tv_pvr_status_t pvrstatus, const char **text,
		  const char **col)
{
  *text = val2str(pvrstatus, recstatustxt);
  if(*text == NULL)
    return -1;

  *col = val2str(pvrstatus, recstatuscolor);
  if(*col == NULL)
    return -1;

  return 0;
}





static void
html_header(tcp_queue_t *tq, const char *title, int javascript)
{
  tcp_qprintf(tq, 
	      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" "
	      "http://www.w3.org/TR/html4/strict.dtd\">\r\n"
	      "<html><head>\r\n"
	      "<title>%s</title>\r\n"
	      "<meta http-equiv=\"Content-Type\" "
	      "content=\"text/html; charset=utf-8\">\r\n", title);

  tcp_qprintf(tq, 
	      "<style type=\"text/css\">\r\n"
	      "<!--\r\n"
	      "img { border: 0px; }\r\n"
	      "a:link { text-decoration: none}\r\n"
	      "a:visited { text-decoration: none}\r\n"
	      "a:active { text-decoration: none}\r\n"
	      "a:link { text-decoration: none; color: #000000}\r\n"
	      "a:visited { text-decoration: none; color: #000000}\r\n"
	      "a:active { text-decoration: none; color: #000000}\r\n"
	      "a:hover { text-decoration: underline; color: CC3333}\r\n"
	      ""
	      "body {margin: 4px 4px; "
	      "font: 75% Verdana, Arial, Helvetica, sans-serif; "
	      "width: 600px; margin-right: auto; margin-left: auto;}\r\n"
	      ""
	      "#box {background: #cccc99;}\r\n"
	      ".roundtop {background: #ffffff;}\r\n"
	      ".roundbottom {background: #ffffff;}\r\n"
	      ".r1{margin: 0 5px; height: 1px; overflow: hidden; "
	      "background: #000000; border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000;}\r\n"
	      ""
	      ".r2{margin: 0 3px; height: 1px; overflow: hidden; "
	      "background: #cccc99; border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000; border-width: 0 2px;}\r\n"
	      ""
	      ".r3{margin: 0 2px; height: 1px; overflow: hidden; "
	      "background: #cccc99; border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000;}\r\n"
	      ""
	      ".r4{margin: 0 1px; height: 2px; overflow: hidden; "
	      "background: #cccc99; border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000;}\r\n"
	      ""
	      ".content3 {padding-left: 3px; height: 60px; "
              "border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000;}\r\n"
	      ""
	      ".content {padding-left: 3px; border-left: 1px solid #000000; "
	      "border-right: 1px solid #000000;}\r\n"
	      ""
	      ".logo {padding: 2px; width: 60px; height: 56px; "
	      "float: left};\r\n"
	      ""
	      ".over {float: left}\r\n"
	      ".toptxt {float: left; width: 165px; text-align: center}\r\n"
	      ""
	      "#meny {margin: 0; padding: 0}\r\n"
	      "#meny li{display: inline; list-style-type: none;}\r\n"
	      "#meny a{padding: 1.15em 0.8em; text-decoration: none;}\r\n"
	      "-->\r\n"
	      "</style>");

  if(javascript) {
    tcp_qprintf(tq, 
		"<script type=\"text/javascript\" "
		"src=\"http://www.olebyn.nu/hts/overlib.js\"></script>\r\n");



    tcp_qprintf(tq,
		"<script language=\"javascript\">\r\n"
		"<!-- Begin\r\n"
		"function epop() {\r\n"
		"  props=window.open(epop.arguments[0],"
		"'poppage', 'toolbars=0, scrollbars=0, location=0, "
		"statusbars=0, menubars=0, resizable=0, "
		"width=600, height=300 left = 100, top = 100');\r\n}\r\n"
		"// End -->\r\n"
		"</script>\r\n");
  }
  /* BODY start */
  
  tcp_qprintf(tq, "</head><body>\r\n");

  if(javascript)
    tcp_qprintf(tq, 
		"<div id=\"overDiv\" style=\"position:absolute; "
		"visibility:hidden; z-index:1000;\"></div>\r\n");

}

static void
html_footer(tcp_queue_t *tq)
{
  tcp_qprintf(tq, 
	      "</body></html>\r\n\r\n");
}


static void
box_top(tcp_queue_t *tq, const char *style)
{
  tcp_qprintf(tq, "<div id=\"%s\">"
	      "<div class=\"roundtop\">"
	      "<div class=\"r1\"></div>"
	      "<div class=\"r2\"></div>"
	      "<div class=\"r3\"></div>"
	      "<div class=\"r4\"></div>"
	      "</div>", style);
}

static void
box_bottom(tcp_queue_t *tq)
{
  tcp_qprintf(tq, 
	      "<div class=\"roundbottom\">"
	      "<div class=\"r4\"></div>"
	      "<div class=\"r3\"></div>"
	      "<div class=\"r2\"></div>"
	      "<div class=\"r1\"></div>"
	      "</div></div>");
}


static void
top_menu(tcp_queue_t *tq)
{
  box_top(tq, "box");

  tcp_qprintf(tq, 
	      "<div class=\"content\">"
	      "<ul id=\"meny\">"
	      "<li><a href=\"/\">TV Guide</a></li>"
	      "<li><a href=\"/pvrlog\">Recordings</a></li>"
	      "<li><a href=\"/status\">System Status</a></li>"
	      "</ul></div>");

  box_bottom(tq);
  tcp_qprintf(tq, "<br>\r\n");
}




void
esacpe_char(char *dst, int dstlen, const char *src, char c, 
	    const char *repl)
{
  char v;
  const char *r;

  while((v = *src++) && dstlen > 1) {
    if(v != c) {
      *dst++ = v;
      dstlen--;
    } else {
      r = repl;
      while(dstlen > 1 && *r) {
	*dst++ = *r++;
	dstlen--;
      }
    }
  }
  *dst = 0;
}



static int
is_client_simple(http_connection_t *hc)
{
  char *c;

  if((c = http_arg_get(&hc->hc_args, "UA-OS")) != NULL) {
    if(strstr(c, "Windows CE") || strstr(c, "Pocket PC"))
      return 1;
  }

  if((c = http_arg_get(&hc->hc_args, "x-wap-profile")) != NULL) {
    return 1;
  }
  return 0;
}


/*
 * Output_event
 */

static void
output_event(http_connection_t *hc, tcp_queue_t *tq, th_channel_t *ch,
	     event_t *e, int simple)
{
  char title[100];
  char bufa[4000];
  char overlibstuff[4000];
  struct tm a, b;
  time_t stop;
  event_t *cur;
  tv_pvr_status_t pvrstatus;
  const char *pvr_txt, *pvr_color;

  localtime_r(&e->e_start, &a);
  stop = e->e_start + e->e_duration;
  localtime_r(&stop, &b);

  pvrstatus = pvr_prog_status(e);

  cur = epg_event_get_current(ch);

  if(!simple && e->e_desc != NULL) {
    esacpe_char(bufa, sizeof(bufa), e->e_desc, '\'', "");
	
    snprintf(overlibstuff, sizeof(overlibstuff), 
	     "onmouseover=\"return overlib('%s')\" "
	     "onmouseout=\"return nd();\"",
	     bufa);
  } else {
    overlibstuff[0] = 0;
  }

  if(1 || simple) {
    snprintf(bufa, sizeof(bufa),
	     "/event/%d", e->e_tag);
  } else {
    snprintf(bufa, sizeof(bufa),
	     "javascript:epop('/event/%d')", e->e_tag);
  }

  esacpe_char(title, sizeof(title), e->e_title, '"', "'");

  tcp_qprintf(tq, 
	      "<div>"
	      "<a href=\"%s\" %s>"
	      "<span style=\"width: %dpx;float: left%s\">"
	      "%02d:%02d - %02d:%02d</span>"
	      "<span style=\"width: %dpx; float: left%s\">%s</span></a>",
	      bufa,
	      overlibstuff,
	      simple ? 80 : 100,
	      e == cur ? ";font-weight:bold" : "",
	      a.tm_hour, a.tm_min, b.tm_hour, b.tm_min,
	      simple ? 100 : 250,
	      e == cur ? ";font-weight:bold" : "",
	      title
	      );

  if(!pvrstatus_to_html(pvrstatus, &pvr_txt, &pvr_color)) {
    tcp_qprintf(tq, 
		"<span style=\"font-style:italic;color:%s;font-weight:bold\">"
		"%s</span></div><br>",
		pvr_color, pvr_txt);
  } else {
    tcp_qprintf(tq, "</div><br>");
  } 
}

/*
 * Root page
 */
static int
page_root(http_connection_t *hc, const char *remain, void *opaque)
{
  tcp_queue_t tq;
  th_channel_t *ch;
  event_t *e;
  int i;
  int simple = is_client_simple(hc);
  

  tcp_init_queue(&tq, -1);

  html_header(&tq, "HTS/tvheadend", !simple);

  top_menu(&tq);

  epg_lock();
  TAILQ_FOREACH(ch, &channels, ch_global_link) {
    box_top(&tq, "box");
    tcp_qprintf(&tq, "<div class=\"content3\">");

    if(!simple) {
      tcp_qprintf(&tq, "<div class=\"logo\">");
      if(ch->ch_icon) {
	tcp_qprintf(&tq, "<a href=\"channel/%d\">"
		    "<img src=\"%s\" height=56px>"
		    "</a>",
		    ch->ch_tag,
		    refstr_get(ch->ch_icon));
      }
      tcp_qprintf(&tq, "</div>");
    }

    tcp_qprintf(&tq, "<div class=\"over\">");
    tcp_qprintf(&tq, "<strong><a href=\"channel/%d\">%s</a></strong><br>",
		ch->ch_tag, ch->ch_name);

    e = epg_event_find_current_or_upcoming(ch);

    for(i = 0; i < 3 && e != NULL; i++) {
      output_event(hc, &tq, ch, e, simple);
      e = TAILQ_NEXT(e, e_link);
    }

    tcp_qprintf(&tq, "</div></div>");
    box_bottom(&tq);
    tcp_qprintf(&tq, "<br>\r\n");
  }
  epg_unlock();

  html_footer(&tq);
  http_output_queue(hc, &tq, "text/html; charset=UTF-8");
  return 0;
}



/*
 * Channel page
 */


const char *days[7] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
  };


static int
page_channel(http_connection_t *hc, const char *remain, void *opaque)
{
  tcp_queue_t tq;
  th_channel_t *ch = NULL;
  event_t *e = NULL;
  int i;
  int simple = is_client_simple(hc);
  int channeltag = -1;
  int w, doff = 0, wday;
  struct tm a;

  i = sscanf(remain, "%d/%d", &channeltag, &doff);
  ch = channel_by_tag(channeltag);
  if(i != 2)
    doff = 0;

  if(ch == NULL) {
    http_error(hc, 404);
    return 0;
  }

  tcp_init_queue(&tq, -1);

  html_header(&tq, "HTS/tvheadend", !simple);

  top_menu(&tq);

  epg_lock();

  box_top(&tq, "box");

  tcp_qprintf(&tq, "<div class=\"content\">");
  tcp_qprintf(&tq, "<strong><a href=\"channel/%d\">%s</a></strong><br>",
	      ch->ch_tag, ch->ch_name);

  e = epg_event_find_current_or_upcoming(ch);
  if(e != NULL) {
    localtime_r(&e->e_start, &a);
    wday = a.tm_wday;


    for(w = 0; w < 7; w++) {

      tcp_qprintf(&tq, 
		  "<a href=\"/channel/%d/%d\""
		  "<u><i>%s</i></u></a><br>",
		  ch->ch_tag, w,
		  days[(wday + w) % 7]);

      while(e != NULL) {
	localtime_r(&e->e_start, &a);
	if(a.tm_wday != wday + w)
	  break;

	if(a.tm_wday == wday + doff)
	  output_event(hc, &tq, ch, e, simple);
	e = TAILQ_NEXT(e, e_link);
      }
    }
  }

  tcp_qprintf(&tq, "</div>");
  box_bottom(&tq);
  tcp_qprintf(&tq, "<br>\r\n");
  epg_unlock();

  html_footer(&tq);
  http_output_queue(hc, &tq, "text/html; charset=UTF-8");
  return 0;
}





/**
 * Event page
 */
static int
page_event(http_connection_t *hc, const char *remain, void *opaque)
{
  tcp_queue_t tq;
  int simple = is_client_simple(hc);
  int eventid = atoi(remain);
  char title[100];
  event_t *e;
  struct tm a, b;
  time_t stop;
  char desc[4000];
  recop_t cmd = 0;
  tv_pvr_status_t pvrstatus;
  const char *pvr_txt, *pvr_color;
  
  epg_lock();
  e = epg_event_find_by_tag(eventid);
  if(e == NULL) {
    epg_unlock();
    return 404;
  }

  remain = strchr(remain, '?');
  if(remain != NULL) {
    remain++;
    if(!strncmp(remain, "rec=", 4))
      cmd = RECOP_ONCE;
    if(!strncmp(remain, "cancel=", 7))
      cmd = RECOP_CANCEL;
    pvr_event_record_op(e->e_ch, e, cmd);

  }

  pvrstatus = pvr_prog_status(e);

  localtime_r(&e->e_start, &a);
  stop = e->e_start + e->e_duration;
  localtime_r(&stop, &b);

  tcp_init_queue(&tq, -1);

  html_header(&tq, "HTS/tvheadend", 0);
  top_menu(&tq);

  tcp_qprintf(&tq, "<form method=\"get\" action=\"/event/%d\">", eventid);


  box_top(&tq, "box");

  tcp_qprintf(&tq, "<div class=\"content\">");


  esacpe_char(title, sizeof(title), e->e_title, '"', "'");
  esacpe_char(desc, sizeof(desc), e->e_desc, '\'', "");

  tcp_qprintf(&tq, 
	      "<div style=\"width: %dpx;float: left;font-weight:bold\">"
	      "%02d:%02d - %02d:%02d</div>"
	      "<div style=\"width: %dpx; float: left;font-weight:bold\">"
	      "%s</div></a>",
	      simple ? 80 : 100,
	      a.tm_hour, a.tm_min, b.tm_hour, b.tm_min,
	      simple ? 100 : 250,
	      title);

  if(!pvrstatus_to_html(pvrstatus, &pvr_txt, &pvr_color))
    tcp_qprintf(&tq, 
		"<div style=\"font-style:italic;color:%s;font-weight:bold\">"
		"%s</div>",
		pvr_color, pvr_txt);
  else
    tcp_qprintf(&tq, "<br>");
    

  tcp_qprintf(&tq, "<br>%s", desc);

  tcp_qprintf(&tq,"<div style=\"text-align: center\">");

  switch(pvrstatus) {
  case HTSTV_PVR_STATUS_SCHEDULED:
  case HTSTV_PVR_STATUS_RECORDING:
    tcp_qprintf(&tq,
		"<input type=\"submit\" name=\"cancel\" "
		"value=\"Cancel recording\">");
    break;


  case HTSTV_PVR_STATUS_NONE:
    tcp_qprintf(&tq,
		"<input type=\"submit\" name=\"rec\" "
		"value=\"Record\">");
    break;

  default:
    tcp_qprintf(&tq,
		"<input type=\"submit\" name=\"cancel\" "
		"value=\"Clear error status\">");
    break;

  }

  tcp_qprintf(&tq, "</div></div></div>");
  box_bottom(&tq);
  tcp_qprintf(&tq, "</form>\r\n");
  html_footer(&tq);
  http_output_queue(hc, &tq, "text/html; charset=UTF-8");

  epg_unlock();

  return 0;
}



static int
pvrcmp(const void *A, const void *B)
{
  const pvr_rec_t *a = *(const pvr_rec_t **)A;
  const pvr_rec_t *b = *(const pvr_rec_t **)B;

  return a->pvrr_start - b->pvrr_start;
}

/**
 * Event page
 */
static int
page_pvrlog(http_connection_t *hc, const char *remain, void *opaque)
{
  tcp_queue_t tq;
  int simple = is_client_simple(hc);
  pvr_rec_t *pvrr;
  event_t *e;
  char escapebuf[4000], href[4000];
  char title[100];
  char channel[100];
  struct tm a, b, day;
  const char *pvr_txt, *pvr_color;
  int c, i;
  pvr_rec_t **pv;

  tcp_init_queue(&tq, -1);
  html_header(&tq, "HTS/tvheadend", 0);
  top_menu(&tq);

  box_top(&tq, "box");

  epg_lock();

  tcp_qprintf(&tq, "<div class=\"content\">");

  c = 0;
  LIST_FOREACH(pvrr, &pvrr_global_list, pvrr_global_link)
    c++;

  pv = alloca(c * sizeof(pvr_rec_t *));

  i = 0;
  LIST_FOREACH(pvrr, &pvrr_global_list, pvrr_global_link)
    pv[i++] = pvrr;


  qsort(pv, i, sizeof(pvr_rec_t *), pvrcmp);

  memset(&day, -1, sizeof(struct tm));

  for(i = 0; i < c; i++) {
    pvrr = pv[i];

    e = epg_event_find_by_time(pvrr->pvrr_channel, pvrr->pvrr_start);

    if(e != NULL && !simple && e->e_desc != NULL) {
      esacpe_char(escapebuf, sizeof(escapebuf), e->e_desc, '\'', "");
    
      snprintf(href, sizeof(href), 
	       "<a href=\"/event/%d\" onmouseover=\"return overlib('%s')\" "
	       "onmouseout=\"return nd();\">",
	       e->e_tag,
	       escapebuf);

    } else {
      href[0] = 0;
    }

    esacpe_char(channel, sizeof(channel), 
		pvrr->pvrr_channel->ch_name, '"', "'");

    esacpe_char(title, sizeof(title), 
		pvrr->pvrr_title ?: "Unnamed recording", '"', "'");

    localtime_r(&pvrr->pvrr_start, &a);
    localtime_r(&pvrr->pvrr_stop, &b);

    if(a.tm_wday  != day.tm_wday  ||
       a.tm_mday  != day.tm_mday  ||
       a.tm_mon   != day.tm_mon   ||
       a.tm_year  != day.tm_year) {

      memcpy(&day, &a, sizeof(struct tm));

      tcp_qprintf(&tq, 
		  "<br><b><i>%s, %d/%d</i></b><br>",
		  days[day.tm_wday],
		  day.tm_mday,
		  day.tm_mon + 1);
    }


    tcp_qprintf(&tq, 
		"%s"
		"<div style=\"width: %dpx;float: left\">"
		"%s</div>"
		"<div style=\"width: %dpx;float: left\">"
		"%02d:%02d - %02d:%02d</div>"
		"<div style=\"width: %dpx; float: left\">%s</div>%s",
		href,
		simple ? 80 : 100,
		channel,
		simple ? 80 : 100,
		a.tm_hour, a.tm_min, b.tm_hour, b.tm_min,
		simple ? 100 : 250,
		title,
		href[0] ? "</a>" : "");

    if(!pvrstatus_to_html(pvrr->pvrr_status, &pvr_txt, &pvr_color)) {
      tcp_qprintf(&tq, 
		  "<div style=\"font-style:italic;color:%s;font-weight:bold\">"
		  "%s</div>",
		  pvr_color, pvr_txt);
    } else {
      tcp_qprintf(&tq, "<br>");
    } 
  }

  epg_unlock();
  tcp_qprintf(&tq, "<br></div>\r\n");

  box_bottom(&tq);
  tcp_qprintf(&tq, "</form>\r\n");
  html_footer(&tq);
  http_output_queue(hc, &tq, "text/html; charset=UTF-8");

  return 0;
}

/**
 * HTML user interface setup code
 */
void
htmlui_start(void)
{
  http_path_add("/", NULL, page_root);
  http_path_add("/event", NULL, page_event);
  http_path_add("/channel", NULL, page_channel);
  http_path_add("/pvrlog", NULL, page_pvrlog);
}