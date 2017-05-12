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

func rootHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("serving User-Agent: ", r.Header.Get("User-Agent"))
	w.Header().Set("Content-Type", "text/html")
	w.WriteHeader(http.StatusOK)
	html := "<html><title>ESP32 Firmware Download Service</title><body><h1>ESP32 Firmware Download Service</h1>"
	html += "<a href=\"/getfirmware\">download firmware</a></body></html>"
	w.Header().Set("Content-Length", fmt.Sprint(len(html)))
	fmt.Fprint(w, html)
}

func firmwareHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("serving User-Agent: ", r.Header.Get("User-Agent"))
	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set("Content-Disposition", "attachment;filename=firmware.bin")
	//data, err := ioutil.ReadFile("C:/Users/bernd/Documents/GitHub/esp32gong/build/esp32gong.bin")
	data, err := ioutil.ReadFile("C:/Users/bernd/Documents/GitHub/ufo-esp32/build/ufo-esp32.bin")
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
		log.Fatal(http.ListenAndServeTLS(":9999", "server.crt", "server.key", nil))
	} else {
		log.Println("server runs at: http://localhost:9999")
		http.HandleFunc("/", rootHandler)
		http.HandleFunc("/getfirmware", firmwareHandler)
		log.Fatal(http.ListenAndServe(":9999", nil))
	}
}
