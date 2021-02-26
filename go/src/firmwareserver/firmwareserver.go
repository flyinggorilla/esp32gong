package main

/*  This firmware server is meant for testing purposes only.
For production use generate proper certificates
*/

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
)

const firmwareFileName string = "C:/Users/bernd/Documents/GitHub/esp32gong/build/esp32gong.bin"

func rootHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("serving User-Agent: ", r.Header.Get("User-Agent"))
	log.Println("request URI: " + r.RequestURI)
	if r.RequestURI == "/version.json" {
		versionHandler(w, r)
		return
	}

	if r.RequestURI == "/esp32gong.bin" {
		firmwareHandler(w, r)
		return
	}

	w.Header().Set("Content-Type", "text/html")
	w.WriteHeader(http.StatusOK)
	html := "<html><title>ESP32 Firmware Download Service</title><body><h1>ESP32 Firmware Download Service</h1>"
	html += "<a href=\"/getfirmware\">download firmware</a></body></html>"
	w.Header().Set("Content-Length", fmt.Sprint(len(html)))
	fmt.Fprint(w, html)
}

func versionHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("VERSIONHANDLER: serving User-Agent: ", r.Header.Get("User-Agent"))
	w.Header().Set("Content-Type", "text/json")

	// get last modified time
	file, err := os.Stat(firmwareFileName)

	if err != nil {
		fmt.Println(err)
	}

	modifiedtime := file.ModTime()

	w.WriteHeader(http.StatusOK)
	json := "{\"version\":\"" + modifiedtime.String() + "\"}"
	w.Header().Set("Content-Length", fmt.Sprint(len(json)))
	fmt.Fprint(w, json)
}

func firmwareHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("FIRMWAREHANDLER: serving User-Agent: ", r.Header.Get("User-Agent"))
	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set("Content-Disposition", "attachment;filename=firmware.bin")
	data, err := ioutil.ReadFile(firmwareFileName)
	if err != nil {
		panic(err)
	}
	w.Header().Set("Content-Length", fmt.Sprint(len(data)))
	log.Println("Content-Length: ", fmt.Sprint(len(data)))
	w.WriteHeader(http.StatusOK)
	fmt.Fprint(w, string(data))
}

func main() {

	if len(os.Args) < 2 {
		log.Println("Please specify commandline option: http | https")
		return
	}

	if os.Args[1] == "https" {
		log.Println("server runs at: https://localhost:9999")
		http.HandleFunc("/", rootHandler)
		http.HandleFunc("/getfirmware", firmwareHandler)
		log.Fatal(http.ListenAndServeTLS(":9999", "server.crt.fortesting", "server.key.fortesting", nil))
	} else {
		log.Println("server runs at: http://localhost:9999")
		http.HandleFunc("/", rootHandler)
		http.HandleFunc("/firmware/esp32gong.bin", firmwareHandler)
		http.HandleFunc("/version/version.json", versionHandler)
		log.Fatal(http.ListenAndServe(":9999", nil))
	}
}
