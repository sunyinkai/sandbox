package json_def

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
