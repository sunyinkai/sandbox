package container_manage

import (
	"bytes"
	"context"
	"fmt"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
	"io"
	"io/ioutil"
	"log"
	"os"
	"sandbox/docker/json_def"
	"sandbox/docker/utils"
	"strings"
	"time"
)

const (
	DOCKERWORKPATH = "/home/sandbox"
	DOCKEREXENAME  = "runner_FSM_docker.out"
)

var cli *client.Client

func init() {
	cli, _ = client.NewClientWithOpts(client.WithVersion("1.40"))
}

type ContainerEntity struct {
	contId     string
	isOccupied bool
	randStr    string // 随机字符串
}

func (ce *ContainerEntity) Reset() {
	ce.isOccupied = false
	ce.randStr = ""
}

//启动容器
func (ce *ContainerEntity) StartContainerEntity(ctx context.Context) {
	if err := cli.ContainerStart(ctx, ce.contId, types.ContainerStartOptions{}); err != nil {
		panic(err.Error())
	}
}

//停止容器
func (ce *ContainerEntity) StopContainer(ctx context.Context) {
	timeout := time.Second * 0 //经过timeout发送kill信号
	if err := cli.ContainerStop(ctx, ce.contId, &timeout); err != nil {
		panic(err.Error())
	}
}

//执行命令
func (ce *ContainerEntity) RunCmdInContainer(ctx context.Context, cmd string) error {
	//创建命令
	var exec = []string{"bash", "-c"}
	exec = append(exec, cmd)

	exeOpt := types.ExecConfig{
		AttachStdout: true,
		AttachStderr: true,
		AttachStdin:  true,
		Tty:          true,
		Cmd:          exec,
	}
	exeId, err := cli.ContainerExecCreate(ctx, ce.contId, exeOpt)
	if err != nil {
		log.Printf(err.Error())
		return err
	}
	log.Printf("exe id is %s", exeId)
	resp, er := cli.ContainerExecAttach(ctx, exeId.ID, types.ExecStartCheck{})
	if er != nil {
		log.Println(er.Error())
		return er
	}
	err = cli.ContainerExecStart(ctx, exeId.ID, types.ExecStartCheck{})
	if err != nil {
		log.Printf("exec error,err is %s", err.Error())
		return err
	}
	_, _ = stdcopy.StdCopy(os.Stdout, os.Stdout, resp.Reader)

	resp.Close()
	return nil
}

//往容器里复制文件
func (ce *ContainerEntity) CopyFileToContainer(ctx context.Context, srcFile string, dstPath string) error {
	var content io.Reader
	targetFile := fmt.Sprintf("%s.tar", srcFile)
	defer os.Remove(targetFile)
	_ = utils.TarFile(srcFile, targetFile)
	content, _ = os.Open(targetFile)
	err := cli.CopyToContainer(ctx, ce.contId, dstPath, content, types.CopyToContainerOptions{AllowOverwriteDirWithFile: true})
	if err != nil {
		log.Println(err.Error())
		return err
	}
	return nil
}

//从容器里拷贝出文件
func (ce *ContainerEntity) CopyFileFromContainer(ctx context.Context, srcFile string, dstPath string) error {
	readCloser, containerPathStatus, err := cli.CopyFromContainer(ctx, ce.contId, srcFile)
	defer readCloser.Close()
	if err != nil {
		log.Println(err.Error())
		return err
	}
	log.Printf("reusult json status %+v", containerPathStatus)
	buf := new(bytes.Buffer)
	_, _ = buf.ReadFrom(readCloser)
	tmpTarFile := fmt.Sprintf("/tmp/result_%s.tar", ce.randStr)
	err = ioutil.WriteFile(tmpTarFile, buf.Bytes(), 0666)
	defer os.Remove(tmpTarFile)
	_ = utils.Untar(tmpTarFile, dstPath)
	if err != nil {
		log.Println(err.Error())
		return err
	}
	return nil
}

//将相关文件传入docker内,构建运行环境
func (ce *ContainerEntity) BuildEnvAndRun(ctx context.Context, args *json_def.CompileAndRunArgs) {
	var err error
	var result json_def.JudgeResult
	//创建临时目录
	absolutePath := fmt.Sprintf("%s/%s", DOCKERWORKPATH, ce.randStr)
	mkdirStr := fmt.Sprintf("mkdir -p %s", absolutePath)
	err = ce.RunCmdInContainer(ctx, mkdirStr)
	if err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "run cmd in docker error")
		return
	}

	//将文件拷贝到docker内去
	if err = ce.CopyFileToContainer(ctx, args.SourceFile, absolutePath); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "copy file to docker error")
		return
	}
	if err = ce.CopyFileToContainer(ctx, args.SysInputFile, absolutePath); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "copy file to docker error")
		return
	}
	if err = ce.CopyFileToContainer(ctx, args.SysOutputFile, absolutePath); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "copy file to docker error")
		return
	}
	// special judge 文件
	if len(args.SpecialJudgeFile) > 0 {
		if err = ce.CopyFileToContainer(ctx, args.SpecialJudgeFile, absolutePath); err != nil {
			result.DumpErrorJson(args.ResultJsonFile, "copy file to docker error")
			return
		}
	}

	//执行相关命令
	chmodStr := fmt.Sprintf("chown root:root -R %s", absolutePath)

	//将dockerExe以及config目录放到absolute目录下
	copyExeAndConfigStr := fmt.Sprintf("cp %s %s && cp -r %s %s", DOCKEREXENAME, absolutePath, "config", absolutePath)
	if err = ce.RunCmdInContainer(ctx, copyExeAndConfigStr); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "run cmd in docker error")
		return
	}

	executableName := fmt.Sprintf("%s/%s", absolutePath, DOCKEREXENAME) //runner_FSM 所在位置
	userExeName := fmt.Sprintf("%s/%s_%s", absolutePath, ce.randStr, args.Language)
	userOutputFile := fmt.Sprintf("%s/%s.output", absolutePath, ce.randStr)
	getFileNameInHost := func(input string) string {
		stringSlice := strings.Split(input, "/")
		return stringSlice[len(stringSlice)-1]
	}
	sysInputFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SysInputFile))
	sysOutputFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SysOutputFile))
	usrSourceFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SourceFile))
	resultJsonFileInDocker := fmt.Sprintf("%s/%s_result.json", DOCKERWORKPATH, ce.randStr)
	var exeRunnerStr string
	if len(args.SpecialJudgeFile) > 0 { //需要spj
		SpecialJudgeFileInDocker := fmt.Sprintf("%s/%s", absolutePath, getFileNameInHost(args.SpecialJudgeFile))
		exeRunnerStr = fmt.Sprintf("%s %s %s %d %d %d %s %s %s %s %s %s",
			executableName, args.Language, usrSourceFileInDocker,
			args.Time, args.Memory, args.Disk,
			userExeName, sysInputFileInDocker, sysOutputFileInDocker, userOutputFile,
			resultJsonFileInDocker, SpecialJudgeFileInDocker)
	} else {
		exeRunnerStr = fmt.Sprintf("%s %s %s %d %d %d %s %s %s %s %s",
			executableName, args.Language, usrSourceFileInDocker,
			args.Time, args.Memory, args.Disk,
			userExeName, sysInputFileInDocker, sysOutputFileInDocker, userOutputFile,
			resultJsonFileInDocker)
	}

	log.Printf("runner Str in docker:%s\n", exeRunnerStr)
	//删除临时文件夹
	rmDirStr := fmt.Sprintf("rm -rf %s", absolutePath)
	unionChmodAndExeRunner := fmt.Sprintf("%s && %s && %s", chmodStr, exeRunnerStr, rmDirStr)
	if err = ce.RunCmdInContainer(ctx, unionChmodAndExeRunner); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "run cmd in docker error")
		return
	}

	//获取生成的json文件
	//resultJsonName := fmt.Sprintf("./test/result_%s.json", utils.GenRandomStr(10)) // for test
	resultJsonName := args.ResultJsonFile
	if err = ce.CopyFileFromContainer(ctx, resultJsonFileInDocker, resultJsonName); err != nil {
		result.DumpErrorJson(args.ResultJsonFile, "get json from docker error")
		return
	}
}
