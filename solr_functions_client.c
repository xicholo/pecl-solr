/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2009 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Israel Ekpo <iekpo@php.net>                                  |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_solr.h"

/* {{{ PHP_SOLR_API int solr_init_options(solr_client_options_t *options TSRMLS_DC) */
PHP_SOLR_API int solr_init_options(solr_client_options_t *options TSRMLS_DC)
{
	solr_string_init(&(options->hostname));
	solr_string_init(&(options->path));
	solr_string_init(&(options->http_auth_credentials));
	solr_string_init(&(options->proxy_hostname));
	solr_string_init(&(options->proxy_auth_credentials));
	solr_string_init(&(options->ssl_cert));
	solr_string_init(&(options->ssl_key));
	solr_string_init(&(options->ssl_keypassword));
	solr_string_init(&(options->ssl_cainfo));
	solr_string_init(&(options->ssl_capath));

	solr_string_init(&(options->qs_delimiter));
	solr_string_init(&(options->response_writer));

	solr_string_init(&(options->update_url));
	solr_string_init(&(options->search_url));
	solr_string_init(&(options->thread_url));
	solr_string_init(&(options->ping_url));
	solr_string_init(&(options->terms_url));

	solr_string_init(&(options->update_servlet));
	solr_string_init(&(options->search_servlet));
	solr_string_init(&(options->thread_servlet));
	solr_string_init(&(options->ping_servlet));
	solr_string_init(&(options->terms_servlet));

	return SUCCESS;
}
/* }}} */


/* {{{ static void solr_set_initial_handle_options(solr_curl_t **sch, solr_client_options_t *options TSRMLS_DC) */
static void solr_set_initial_curl_handle_options(solr_curl_t **sch_ptr, solr_client_options_t *options TSRMLS_DC)
{
	solr_curl_t * sch = *sch_ptr;

	/** Setup all the required CURL options here **/
	curl_easy_setopt(sch->curl_handle, CURLOPT_NOPROGRESS,        1L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_VERBOSE,           1L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_ERRORBUFFER,       sch->err.str);
	curl_easy_setopt(sch->curl_handle, CURLOPT_WRITEFUNCTION,     solr_curl_write);
	curl_easy_setopt(sch->curl_handle, CURLOPT_WRITEDATA,         (void *) sch);
	curl_easy_setopt(sch->curl_handle, CURLOPT_HEADERFUNCTION,    solr_curl_write_header);
	curl_easy_setopt(sch->curl_handle, CURLOPT_HEADERDATA,        (void *) sch);
	curl_easy_setopt(sch->curl_handle, CURLOPT_DEBUGFUNCTION,     solr_curl_debug_callback);
	curl_easy_setopt(sch->curl_handle, CURLOPT_DEBUGDATA,         (void *) sch);

	curl_easy_setopt(sch->curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

/**
#if LIBCURL_VERSION_NUM >= 0x071304
	curl_easy_setopt(sch->curl_handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
#endif
*/
	curl_easy_setopt(sch->curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 120L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_MAXREDIRS, 16L); /* prevent infinite redirects  */
	curl_easy_setopt(sch->curl_handle, CURLOPT_UNRESTRICTED_AUTH, 0L);

#ifdef ZTS
	curl_easy_setopt(sch->curl_handle, CURLOPT_NOSIGNAL, 1L); /** Needed in multi-threaded environments **/
#endif

	curl_easy_setopt(sch->curl_handle, CURLOPT_TIMEOUT, options->timeout);

	curl_easy_setopt(sch->curl_handle, CURLOPT_USERAGENT, SOLR_CLIENT_USER_AGENT);

	/* If the login and password options are passed, send the authentication headers */
	if (options->http_auth_credentials.len) {

		curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

		curl_easy_setopt(sch->curl_handle, CURLOPT_USERPWD, options->http_auth_credentials.str);
	}

	/* If there is proxy info, send it too */
	if (options->proxy_hostname.len) {

		curl_easy_setopt(sch->curl_handle, CURLOPT_PROXY, options->proxy_hostname.str);

		if (options->proxy_port) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_PROXYPORT, options->proxy_port);
		}

		if (options->proxy_auth_credentials.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_PROXYUSERPWD, options->proxy_auth_credentials.str);
		}
	}

	 /*
	 * When negotiating an SSL connection, the server sends a certificate indicating its identity.
	 * cURL verifies whether the certificate is authentic
	 */
	if (options->secure) {

		/* The name of the PEM-formatted private key and private certificate concatenated */
		if (options->ssl_cert.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_SSLCERT, options->ssl_cert.str);

			curl_easy_setopt(sch->curl_handle, CURLOPT_SSLCERTTYPE, "PEM");
		}

		/* The name of the PEM-formatted private key, if certificate and private key are separate */
		if (options->ssl_key.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_SSLKEY, options->ssl_key.str);

			curl_easy_setopt(sch->curl_handle, CURLOPT_SSLKEYTYPE, "PEM");
		}

		/* Password for the PEM-formatted private key */
		if (options->ssl_keypassword.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_KEYPASSWD, options->ssl_keypassword.str);
		}

		/* The file holding one or more CA certificates to verify the peer with */
		if (options->ssl_cainfo.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_CAINFO, options->ssl_cainfo.str);
		}

		/* The directory holding multiple CA certificates to verify the peer with */
		if (options->ssl_capath.len) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_CAPATH , options->ssl_capath.str);
		}

		/*
		 * Whether curl verifies the authenticity of the host name of server
		 * Based on the Common name section of the certificate
		 */
		if (options->ssl_verify_peer && options->ssl_verify_host) {

			curl_easy_setopt(sch->curl_handle, CURLOPT_SSL_VERIFYHOST, options->ssl_verify_host);
		}

		/*
		 * Verify the authenticity of the peer's certificate
		 * This authentication is based on a chain of digital signatures,
		 * rooted in certification authority (CA) certificates.
		 *
		 * If something is not right, the connection will not be valid
		 */
		curl_easy_setopt(sch->curl_handle, CURLOPT_SSL_VERIFYPEER, options->ssl_verify_peer);
	}
}
/* }}} */

/* {{{ PHP_SOLR_API int solr_init_handle(solr_curl_t *sch, solr_client_options_t *options TSRMLS_DC) */
PHP_SOLR_API int solr_init_handle(solr_curl_t *sch, solr_client_options_t *options TSRMLS_DC)
{
	sch->response_header.response_code = 0L;

	memset(sch->err.str, 0, sizeof(sch->err.str));

	sch->curl_handle = curl_easy_init();

	if (NULL == sch->curl_handle) {

		return FAILURE;
	}

	sch->result_code = CURLE_OK;

	sch->handle_status = 1;

#ifdef ZTS
	sch->tsrm_ls = TSRMLS_C;
#endif

	solr_string_init(&(sch->request_header.buffer));
	solr_string_init(&(sch->request_body.buffer));
	solr_string_init(&(sch->request_body_debug.buffer));
	solr_string_init(&(sch->response_header.buffer));
	solr_string_init(&(sch->response_body.buffer));
	solr_string_init(&(sch->debug_data_buffer));

	solr_set_initial_curl_handle_options(&(sch), options TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/**
 * solr_curl_write
 *
 * Called by libcurl as soon as there is response body data received that needs to be saved.
 *
 * The size of the data pointed to by *data is size multiplied with nmemb, it will not be zero terminated
 *
 * The *ctx parameter is the solr_curl pointer
 */
/* {{{ size_t solr_curl_write(solr_char_t *data, size_t size, size_t nmemb, void *ctx) */
size_t solr_curl_write(solr_char_t *data, size_t size, size_t nmemb, void *ctx)
{
	size_t length = (size * nmemb);
	solr_curl_t *sch = (solr_curl_t *) ctx;

	solr_string_appends(&(sch->response_body.buffer), data, length);

	return length;
}
/* }}} */

/**
 * solr_curl_write_header
 *
 * Called by libcurl as soon as it has received header data
 *
 * It will be called once for each header and only complete header lines are passed on to the callback.
 *
 * Note: header lines may not be null-terminated so beware.
 *
 * The *ctx parameter is the solr_curl pointer
 */
/* {{{ size_t solr_curl_write_header(solr_char_t *data, size_t size, size_t nmemb, void *ctx) */
size_t solr_curl_write_header(solr_char_t *data, size_t size, size_t nmemb, void *ctx)
{
	size_t length = (size * nmemb);
	solr_curl_t *sch = (solr_curl_t *) ctx;

	solr_string_appends(&(sch->response_header.buffer), data, length);

	return length;
}
/* }}} */

/**
 * solr_curl_debug_callback
 *
 * The infotype parameter is one of :
 *
 * 0 = CURLINFO_TEXT
 * 1 = CURLINFO_HEADER_IN   (Response Headers)
 * 2 = CURLINFO_HEADER_OUT  (Request Headers)
 * 3 = CURLINFO_DATA_IN		(Response Body)
 * 4 = CURLINFO_DATA_OUT	(Request Body)
 * 5 = CURLINFO_SSL_DATA_IN
 * 6 = CURLINFO_SSL_DATA_OUT
 *
 * The debug_data parameter is not null-terminated but is exactly size characters
 *
 * The ctx parameter is a solr_curl * pointer
 */
/* {{{ int solr_curl_debug_callback(CURL *curl_handle, curl_infotype infotype, solr_char_t *debug_data, size_t size, void *ctx) */
int solr_curl_debug_callback(CURL *curl_handle, curl_infotype infotype, solr_char_t *debug_data, size_t size, void *ctx)
{
	solr_curl_t *sch = (solr_curl_t *) ctx;

	/* TODO : Check why this function is still called after destructor has been called */
	if (!sch->handle_status)
	{
		return 0;
	}

	switch(infotype)
	{
		case CURLINFO_HEADER_OUT : /* Capture the Actual Request Headers Sent to Server */
		{
			solr_string_appends(&(sch->request_header.buffer), debug_data, size);
		}
		break;

		case CURLINFO_DATA_OUT : /* Capture the Actual Request Body Sent to Server */
		{
			solr_string_appends(&(sch->request_body_debug.buffer), debug_data, size);
		}
		break;

		default :
		{
			/* do nothing */
		}
		break;
	}

    /* Captures ALL debug information */
	solr_string_appends(&(sch->debug_data_buffer), debug_data, size);

	return 0;
}
/* }}} */

/* {{{ PHP_SOLR_API int solr_make_request(solr_client_t *client, solr_request_type_t request_type TSRMLS_DC) */
PHP_SOLR_API int solr_make_request(solr_client_t *client, solr_request_type_t request_type TSRMLS_DC)
{
	solr_curl_t *sch = &(client->handle);
	solr_client_options_t *options = &(client->options);
	solr_http_header_list_t *header_list = NULL;
	int return_status = SUCCESS;
	CURLcode info_status = CURLE_OK;

	header_list = curl_slist_append(header_list, "Accept-Charset: utf-8");
	header_list = curl_slist_append(header_list, "Keep-Alive: 300");
	header_list = curl_slist_append(header_list, "Connection: keep-alive");

	/* Disable the Expect: 100-continue header. Jetty gets confused with this header */
	header_list = curl_slist_append(header_list, "Expect:");

	/* Reset the buffers */
	solr_string_free(&sch->request_header.buffer);
	solr_string_free(&sch->request_body_debug.buffer);
	solr_string_free(&sch->response_body.buffer);
	solr_string_free(&sch->response_header.buffer);
	solr_string_free(&sch->debug_data_buffer);

	curl_easy_reset(sch->curl_handle);

	solr_set_initial_curl_handle_options(&(sch), options TSRMLS_CC);

	/* Reset the CURL options if the handle is reused */
	curl_easy_setopt(sch->curl_handle, CURLOPT_HEADER,  0L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_POST,    0L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPGET, 0L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_NOBODY,  0L);

	curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDSIZE, 0L);
	curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDS, NULL);
	curl_easy_setopt(sch->curl_handle, CURLOPT_URL, NULL);
	curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, NULL);

	switch(request_type)
	{
		case SOLR_REQUEST_SEARCH : /* HTTP FORM POST */
		{
			header_list = curl_slist_append(header_list, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

			curl_easy_setopt(sch->curl_handle, CURLOPT_POST,    1L);

			curl_easy_setopt(sch->curl_handle, CURLOPT_URL, options->search_url.str);
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, header_list);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDSIZE, sch->request_body.buffer.len);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDS, sch->request_body.buffer.str);
		}
		break;

		case SOLR_REQUEST_TERMS : /* HTTP FORM POST */
		{
			header_list = curl_slist_append(header_list, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

			curl_easy_setopt(sch->curl_handle, CURLOPT_POST,    1L);

			curl_easy_setopt(sch->curl_handle, CURLOPT_URL, options->terms_url.str);
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, header_list);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDSIZE, sch->request_body.buffer.len);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDS, sch->request_body.buffer.str);
		}
		break;

		case SOLR_REQUEST_UPDATE : /* HTTP XML POST */
		{
			header_list = curl_slist_append(header_list, "Content-Type: text/xml;charset=UTF-8");

			curl_easy_setopt(sch->curl_handle, CURLOPT_POST,    1L);

			curl_easy_setopt(sch->curl_handle, CURLOPT_URL, options->update_url.str);
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, header_list);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDSIZE, sch->request_body.buffer.len);
			curl_easy_setopt(sch->curl_handle, CURLOPT_POSTFIELDS, sch->request_body.buffer.str);

		}
		break;

		case SOLR_REQUEST_THREADS : /* HTTP GET */
		{
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPGET, 1L);
			curl_easy_setopt(sch->curl_handle, CURLOPT_URL, options->thread_url.str);
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, header_list);

		}
		break;

		case SOLR_REQUEST_PING :	/* HTTP HEAD REQUEST. */
		{
			curl_easy_setopt(sch->curl_handle, CURLOPT_HEADER,  1L);
			curl_easy_setopt(sch->curl_handle, CURLOPT_NOBODY,  1L);

			curl_easy_setopt(sch->curl_handle, CURLOPT_URL, options->ping_url.str);
			curl_easy_setopt(sch->curl_handle, CURLOPT_HTTPHEADER, header_list);
		}
		break;

		default :
		{
			return_status = FAILURE;
		}
		break;
	}

	sch->result_code = curl_easy_perform(sch->curl_handle);

	info_status = curl_easy_getinfo(sch->curl_handle, CURLINFO_RESPONSE_CODE, &(sch->response_header.response_code));

	if (info_status != CURLE_OK)
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "HTTP Transfer status could not be retrieved successfully");

		return_status = FAILURE;
	}

	if (sch->result_code != CURLE_OK)
	{
		/* @TODO change this to an exception */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Solr HTTP Error : '%s' ", curl_easy_strerror(sch->result_code));

		return_status = FAILURE;
	}

	if (sch->response_header.response_code != 200L)
	{
		return_status = FAILURE;
	}

	curl_slist_free_all(header_list);

	return return_status;
}
/* }}} */


/** Deallocates memory associated with the Solr CURL Handle. **/
/* {{{ PHP_SOLR_API void solr_free_handle(solr_curl_t *sch) */
PHP_SOLR_API void solr_free_handle(solr_curl_t *sch)
{
	solr_string_free(&((sch)->request_header.buffer));
	solr_string_free(&((sch)->request_body.buffer));
	solr_string_free(&((sch)->request_body_debug.buffer));
	solr_string_free(&((sch)->response_header.buffer));
	solr_string_free(&((sch)->response_body.buffer));
	solr_string_free(&((sch)->debug_data_buffer));

	sch->handle_status = 0;

    curl_easy_cleanup((sch)->curl_handle);

    sch->curl_handle = NULL;
}
/* }}} */

/* {{{ PHP_SOLR_API void solr_free_options(solr_client_options_t *options) */
PHP_SOLR_API void solr_free_options(solr_client_options_t *options)
{
	solr_string_free(&((options)->hostname));
	solr_string_free(&((options)->http_auth_credentials));
	solr_string_free(&((options)->path));
	solr_string_free(&((options)->proxy_auth_credentials));
	solr_string_free(&((options)->ssl_cert));
	solr_string_free(&((options)->ssl_key));
	solr_string_free(&((options)->ssl_keypassword));
	solr_string_free(&((options)->ssl_cainfo));
	solr_string_free(&((options)->ssl_capath));
	solr_string_free(&((options)->proxy_hostname));

	solr_string_free(&((options)->qs_delimiter));
	solr_string_free(&((options)->response_writer));

	solr_string_free(&((options)->update_url));
	solr_string_free(&((options)->search_url));
	solr_string_free(&((options)->thread_url));
	solr_string_free(&((options)->ping_url));
	solr_string_free(&((options)->terms_url));

	solr_string_free(&((options)->update_servlet));
	solr_string_free(&((options)->search_servlet));
	solr_string_free(&((options)->thread_servlet));
	solr_string_free(&((options)->ping_servlet));
	solr_string_free(&((options)->terms_servlet));
}
/* }}} */

/* {{{ PHP_SOLR_API void solr_destroy_client(void *client) */
PHP_SOLR_API void solr_destroy_client(void *client)
{
	solr_client_t *solr_client = (solr_client_t *) client;

	if (solr_client)
	{
		solr_free_options(&(solr_client->options));

		solr_free_handle(&(solr_client->handle));

		solr_client = NULL;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
