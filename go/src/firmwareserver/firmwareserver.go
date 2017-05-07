package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

func rootHandler(w http.ResponseWriter, r *http.Request) {
	log.Println("serving User-Agent: ", r.Header.Get("User-Agent"))
	w.Header().Set("Content-Type", "application/octet-stream")
	w.Header().Set("Content-Disposition", "attachment;filename=firmware.bin")
	w.WriteHeader(http.StatusOK)
	data, err := ioutil.ReadFile("C:/Users/bernd/Documents/GitHub/esp32gong/build/esp32gong.bin")
	if err != nil {
		panic(err)
	}
	w.Header().Set("Content-Length", fmt.Sprint(len(data)))
	fmt.Fprint(w, string(data))
}

func main() {
	log.Println("server runs at: http://localhost:9999/getfirmware")
	http.HandleFunc("/getfirmware", rootHandler)
	log.Fatal(http.ListenAndServe(":9999", nil))
}
