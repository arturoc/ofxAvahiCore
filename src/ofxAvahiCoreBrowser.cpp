/*
 * ofxAvahiCoreBrowser.cpp
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#include "ofxAvahiCoreBrowser.h"
#include "ofLog.h"
#include "ofUtils.h"
#include "ofAppRunner.h"

string ofxAvahiCoreBrowser::LOG_NAME="ofxAvahiCoreBrowser";

ofxAvahiCoreBrowser::ofxAvahiCoreBrowser():
sb(NULL),
poll(NULL),
server(NULL)
{


}

ofxAvahiCoreBrowser::~ofxAvahiCoreBrowser() {
	close();
}

bool ofxAvahiCoreBrowser::lookup(string _type){
	AvahiServerConfig config;
	int error;
	type = _type;

	if (!(poll = avahi_simple_poll_new())) {
		ofLogError(LOG_NAME) << "Failed to create simple poll object.";
		close();
		return false;
	}
	/* Do not publish any local records */
	avahi_server_config_init(&config);
	config.publish_hinfo = 0;
	config.publish_addresses = 0;
	config.publish_workstation = 0;
	config.publish_domain = 0;
	config.use_ipv6 = 0;
	config.disallow_other_stacks = 1;
	config.allow_point_to_point = 1;

	/* Set a unicast DNS server for wide area DNS-SD */
    /*avahi_address_parse("192.168.50.1", AVAHI_PROTO_UNSPEC, &config.wide_area_servers[0]);
    config.n_wide_area_servers = 1;
    config.enable_wide_area = 1;*/

	server = avahi_server_new(avahi_simple_poll_get(poll), &config, (AvahiServerCallback)server_cb, this, &error);

	avahi_server_config_free(&config);

	if (!server) {
		ofLogError(LOG_NAME) << "Failed to create server:" << avahi_strerror(error);
		close();
		return false;
	}


    /* Create the service browser */
    if (!(sb = avahi_s_service_browser_new(server, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, type.c_str(), NULL, (AvahiLookupFlags)0, (AvahiSServiceBrowserCallback)browse_cb, this))) {
    	ofLogError(LOG_NAME) << "Failed to create service browser:" << avahi_strerror(avahi_server_errno(server));
    	close();
    	return false;
    }

	startThread(true,false);

	return true;
}

void ofxAvahiCoreBrowser::threadedFunction(){
	if(avahi_simple_poll_loop(poll)){
		ofLogError(LOG_NAME) << "error in poll loop";
	}else{
		ofLogVerbose(LOG_NAME) << "poll loop finished";
	}
}

void ofxAvahiCoreBrowser::close(){
	ofLogVerbose(LOG_NAME) << "closing";

    if (poll){
        avahi_simple_poll_quit(poll);
        waitForThread(false);
    }

	if (sb)
		avahi_s_service_browser_free(sb);

	if (server)
		avahi_server_free(server);

    if (poll)
        avahi_simple_poll_free(poll);
}

void ofxAvahiCoreBrowser::server_cb (AvahiServer *s, AvahiServerState state, ofxAvahiCoreBrowser* browser){
	switch(state){
	case AVAHI_SERVER_INVALID:          /**< Invalid state (initial) */
		ofLogError(LOG_NAME) << "server state changed to invalid";
        browser->close();
		break;
	case AVAHI_SERVER_REGISTERING:      /**< Host RRs are being registered */
		ofLogNotice(LOG_NAME) << "server state changed to registering";
		break;
	case AVAHI_SERVER_RUNNING:          /**< All host RRs have been established */
		ofLogNotice(LOG_NAME) << "server state changed to running";
	    /* Create the service browser */
	    /*if (!(browser->sb = avahi_s_service_browser_new(s, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, browser->type.c_str(), NULL, (AvahiLookupFlags)0, (AvahiSServiceBrowserCallback)browse_cb, browser))) {
	    	ofLogError(LOG_NAME) << "Failed to create service browser:" << avahi_strerror(avahi_server_errno(s));
	    	browser->close();
	    }*/
		break;
	case AVAHI_SERVER_COLLISION:        /**< There is a collision with a host RR. All host RRs have been withdrawn, the user should set a new host name via avahi_server_set_host_name() */
		ofLogNotice(LOG_NAME) << "server state changed to collision";
		break;
	case AVAHI_SERVER_FAILURE:          /**< Some fatal failure happened, the server is unable to proceed */
		ofLogError(LOG_NAME) << "server state changed to failure";
        browser->close();
		break;
	}
}

void ofxAvahiCoreBrowser::browse_cb(
    AvahiSServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    ofxAvahiCoreBrowser* browser) {

    AvahiServer *s = browser->server;
    assert(b);

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

    switch (event) {

        case AVAHI_BROWSER_FAILURE:
        	ofLogError(LOG_NAME) <<  "(Browser)" << avahi_strerror(avahi_server_errno(browser->server));
            browser->close();
            return;

        case AVAHI_BROWSER_NEW:
            ofLogNotice(LOG_NAME) << "(Browser) NEW: service '"<< name << "' of type '" << type<<"' in domain '"<<domain<<"'";

            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */

            if (!(avahi_s_service_resolver_new(s, interface, protocol, name, type, domain, AVAHI_PROTO_INET, (AvahiLookupFlags)0, (AvahiSServiceResolverCallback)resolve_cb, browser)))
                ofLogError(LOG_NAME) << "Failed to resolve service '" << name << "':"<<avahi_strerror(avahi_server_errno(s));

            break;

        case AVAHI_BROWSER_REMOVE:{
        	ofLogNotice(LOG_NAME) << "(Browser) REMOVE: service '" << name << "' of type '"<< type << "' in domain '"<< domain << "'";
            ofxAvahiService service;
            service.domain = domain;
            service.name = name;
            ofNotifyEvent(browser->serviceRemoveE,service);
        }break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
        	ofLogNotice(LOG_NAME) << "(Browser)" << ((event == AVAHI_BROWSER_CACHE_EXHAUSTED) ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
            break;
    }
}

void ofxAvahiCoreBrowser::resolve_cb(
    AvahiSServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    ofxAvahiCoreBrowser* browser) {

    assert(r);

    /* Called whenever a service has been resolved successfully or timed out */

    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            ofLogError(LOG_NAME) << "(Resolver) Failed to resolve service '" << name << "' of type '" << type << "' in domain '" << domain << "':" << avahi_strerror(avahi_server_errno(browser->server));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX], *t;

            ofLogNotice(LOG_NAME) << "(Resolver) Service '" << name << "' of type '" << type << "' in domain '" << domain;

            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            ofLogNotice(LOG_NAME) << ofVAArgsToString(
                    "\t%s:%u (%s)\n"
                    "\tTXT=%s\n"
                    "\tcookie is %u\n"
                    "\tis_local: %i\n"
                    "\twide_area: %i\n"
                    "\tmulticast: %i\n"
                    "\tcached: %i\n",
                    host_name, port, a,
                    t,
                    avahi_string_list_get_service_cookie(txt),
                    !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
                    !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
                    !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
                    !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

			ofxAvahiService service;
			service.domain = domain;
			service.host_name = host_name;
			service.ip = a;
			service.name = name;
			service.port = port;
			ofNotifyEvent(browser->serviceNewE,service);

            avahi_free(t);
            break;
        }
    }

    avahi_s_service_resolver_free(r);
}

