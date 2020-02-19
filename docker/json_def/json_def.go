package json_def

type CompileAndRunArgs struct {
	Language     string `json:"language"`
	Time         int    `json:"time"`
	Memory       int64  `json:"memory"`
	Disk         int64  `json:"disk"`
	SysInputFile string `json:"sysInputFile"`
	SourceFile   string `json:"sourceFile"`
}
