/*
 * ofxAvahiCoreBrowser.h
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#ifndef OFXAVAHICOREBROWSER_H_
#define OFXAVAHICOREBROWSER_H_

#include <avahi-core/core.h>
#include <avahi-core/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include "ofThread.h"
#include "ofEvents.h"
#include "ofConstants.h"

struct ofxAvahiService{
	string name;
	string host_name;
	string domain;
	string ip;
	int port;

};

class ofxAvahiCoreBrowser: public ofThread {
public:
	ofxAvahiCoreBrowser();
	virtual ~ofxAvahiCoreBrowser();

	bool lookup(string type);
	void close();

	static string LOG_NAME;


	ofEvent<ofxAvahiService> serviceNewE;
	ofEvent<ofxAvahiService> serviceRemoveE;

protected:
	void threadedFunction();

private:
	AvahiSServiceBrowser *sb;
	AvahiSimplePoll * poll;
	AvahiServer *server;

	static void browse_cb(
		    AvahiSServiceBrowser *b,
		    AvahiIfIndex interface,
		    AvahiProtocol protocol,
		    AvahiBrowserEvent event,
		    const char *name,
		    const char *type,
		    const char *domain,
		    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
		    ofxAvahiCoreBrowser* userdata);

	static void resolve_cb(
		    AvahiSServiceResolver *r,
		    AVAHI_GCC_UNUSED AvahiIfIndex interface,
		    AVAHI_GCC_UNUSED AvahiProtocol protocol,
		    AvahiResolverEvent event,
		    const char *name,
		    const char *type,
		    const char *domain,
		    const char *host_name,
		    const AvahiAddress *address,
		    uint16_t port,
		    AvahiStringList *txt,
		    AvahiLookupResultFlags flags,
		    AVAHI_GCC_UNUSED ofxAvahiCoreBrowser* userdata);
};

#endif /* OFXAVAHICOREBROWSER_H_ */
