package main

import (
	"io/ioutil"
	"sandbox/docker/mq"
)

func main() {
	mq.Init()
	jsonByte, err := ioutil.ReadFile("config.json")
	if err != nil {
		panic(err)
	}
	mq.RedisCli.LPush(mq.MESSAGEQUEUEKEY, jsonByte)
}
