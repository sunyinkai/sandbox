package json_def

import (
	"encoding/json"
	"io/ioutil"
)

type CompileAndRunArgs struct {
	Language       string `json:"language"`
	Time           int    `json:"time"`
	Memory         int64  `json:"memory"`
	Disk           int64  `json:"disk"`
	SysInputFile   string `json:"sysInputFile"`
	SysOutputFile  string `json:"sysOutputFile"`
	SourceFile     string `json:"sourceFile"`
	ResultJsonFile string `json:"resultJsonFile"`
}

type JudgeResult struct {
	TimeUsage    int64  `json:"timeUsage"`
	MemoryUsage  int64  `json:"memoryUsage"`
	SystemStatus int    `json:"systemStatus"`
	JudgeStatus  int    `json:"judgeStatus"`
	ResultString string `json:"resultString"`
	Reason       string `json:"reason"`
}

func (jr *JudgeResult) DumpErrorJson(path, reason string) {
	jr.SystemStatus = 1 //EXIT_SYSTEM_ERROR
	if reason == "" {
		jr.Reason = "system error"
	}
	jr.Reason = reason
	resultString, err := json.Marshal(jr)
	if err != nil {
		_ = ioutil.WriteFile(path, resultString, 0664)
	}
}
