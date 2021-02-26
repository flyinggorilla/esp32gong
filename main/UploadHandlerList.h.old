#ifndef MAIN_UPLOADHANDLERLIST_H_
#define MAIN_UPLOADHANDLERLIST_H_

#include <list>


class String;
class DownAndUploadHandler;


class UploadHandlerEntry {
    public:
        /* register multiple File Upload handlers. e.g. one for Ota, an otherone for file system, another one for streaming 
         * @param: sUrl             URL like "/upload"
         * @param: pUploadHandler   Uploadhandler to use for given URL; if NULL, upload will be kept in memory in message body
         */
        void UploadHandlerEntry(String sUrl, DownAndUploadHandler* pUploadHandler = NULL) {
            msUrl = sUrl;
            mpUploadHandler = pUploadHandler;
        }
          
        String msUrl;
        DownAndUploadHandler* mpUploadHandler = NULL;

};

class UploadHandlerList {
public:
    std::list<UploadHandlerEntry> mEntries;
};


#endif // MAIN_UPLOADHANDLERLIST_H_
