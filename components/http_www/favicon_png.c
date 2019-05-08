#include "http.h"

static const char  fileStart[]	asm("_binary_favicon_png_start");
static const char  fileEnd[]	asm("_binary_favicon_png_end");

static const httpFile_t httpFile = {
	.uri	= "/favicon.png",
	.start	= fileStart,
	.end	= fileEnd,
	.type	= "image/png"
};

void httpFileFaviconPNGInit(httpd_handle_t server) {
	httpFileRegister(server, &httpFile);
}