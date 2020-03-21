package mq

import "github.com/go-redis/redis"

var RedisCli *redis.Client

const MESSAGEQUEUEKEY = "jsonmq"

func Init() {
	RedisCli = redis.NewClient(&redis.Options{
		Addr:     "localhost:6379",
		Password: "",
		DB:       0,
	})
}
