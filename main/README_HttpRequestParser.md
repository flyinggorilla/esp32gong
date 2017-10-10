## Extended **multipart/form-data** parsing in **HttpRequestParser**

---

### Usage

1. `Prepare form in html`_.
2. `Receive POST data from the form`_.
3. `Process POST data in **DynamicRequestHandler**`_.

---

#### Prepare form in html

There are three ``input type`` fields in the following form example. The ``file`` field must be the **last** field in the form. The ``curdir`` field can be used for sending *current directory* to the server. The ``dummy`` field is just an example extra field. There is **phonon** example page with form:

`
<pagesdcard data-page="true">
	<header class="header-bar">
		<button class="btn icon icon-arrow-back pull-left" data-navigation="$previous-page"></button>
		<div class="center">
			<h6 class="title" data-i18n="text: SDCard"></h6>
		</div>
	</header>
	<div class="content">
		<div class="padded-full">
			<ul class="list">
				<li class="divider" id="curdir"></li>
				<div id='dirlist' data-i18n="text: Loading">
				</div>
			</ul>
		</div>
		<div class="padded-full">
			<!-- action as in ::OnReceiveBegin() -->
			<form method='POST' action='/uploadtodev' enctype='multipart/form-data' id='uploadform'>
				<ul class="list">
					<li class="divider" data-i18n="text: UploadToDev"></li>
					<span class="padded-list" data-i18n="text: SelectFile"></span>
					<li>
						<input type='hidden' name='curdir' id='uploaddir'/>
						<input type='hidden' name='dummy' id='uploaddummy'/>
						<input class="padded-full" type="file" name="userfile" id='uploadfilename' accept=".*"/>
					</li>
					<li>
						<span class="padded-list"><font color="red" id="uploaderror"></font></span>
					</li>
					<li>
						<button class="btn btn-submit primary" type='button' id='uploadbtn' data-i18n="text: Upload"></button>
					</li>
				</ul>
			</form>
		</div>
	</div>
</pagesdcard>
`

#### Receive POST data from the form

To receive POST data add ``handler`` to the ``HandleRequest`` mwthod in *Esp32GongWebServer.cpp*::

	else if (httpParser.GetUrl().equals("/uploadtodev")){
		if (!requestHandler.HandleUploadToDevRequest(httpParser.GetParams(), httpResponse))
			return false;
	} 

The ``handler`` processes the path ``/uploadtodev`` which is set as an ``action`` in the form tag.

#### Process POST data in **DynamicRequestHandler**

Release ``HandleUploadToDevRequest`` method in *DynamicRequestHandler.cpp* using template:

`
bool DynamicRequestHandler::HandleUploadToDevRequest(std::list<TParam>& params, HttpResponse& rResponse){
	String sCurDir;
	String sFileName;
	String sDummy;
	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()){
		if ((*it).paramName == "curdir")
			sCurDir = (*it).paramValue.c_str();
		else if ((*it).paramName == "dummy")
			sDummy = (*it).paramValue.c_str();
		else if ((*it).paramName == "filename")
			sFileName = (*it).paramValue.c_str();
		it++;
	}
	if (sCurDir && sFileName) {
		sCurDir.trim();
		sFileName.trim();
		ESP_LOGD(TAG, "%s() sCurDir=\"%s\" sDummy=\"%s\" sFileName=\"%s\"", __func__, sCurDir.c_str(), sDummy.c_str(), sFileName.c_str());
		//if (sFileName.length() > 0) {
		//	if (pmySD->renameDownloadTmpFile(sCurDir.c_str(), sFileName.c_str())) {
		//		ESP_LOGD(TAG, "%s() tmp file successfully renamed, sCurDir=\"%s\" sFileName=\"%s\"", __func__, sCurDir.c_str(), sFileName.c_str());
		//	} else {
		//		ESP_LOGE(TAG, "%s() tmp file failed to rename, sCurDir=\"%s\" sFileName=\"%s\" errno=%d(%xh)", __func__, sCurDir.c_str(), sFileName.c_str(), errno, errno);
		//	}
		//} else {
		//	ESP_LOGE(TAG, "%s() sCurDir=\"%s\" sFileName=\"%s\" (empty, ignore renaming)", __func__, sCurDir.c_str(), sFileName.c_str());
		//}
	} else {
		ESP_LOGD(TAG, "%s() p=NULL", __func__);
	}
	//pmySD->eraseDownloadTmpFile();
	String sLocation("Location: /#!pagesdcard");
	//sLocation += sCurDir;
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.AddHeader(sLocation.c_str());
	rResponse.SetRetCode(302);
	return rResponse.Send();
}
`


### How to build

To build project with extended feature set macro ``EXTRA_PARSING`` in *HttpRequestParser.h* to 1 (or to 0 to disable).

