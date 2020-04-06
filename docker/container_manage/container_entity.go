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
		panic(err)
	}
}

//停止容器
func (ce *ContainerEntity) StopContainer(ctx context.Context) {
	timeout := time.Second * 0 //经过timeout发送kill信号
	if err := cli.ContainerStop(ctx, ce.contId, &timeout); err != nil {
		panic(err)
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
		fmt.Println(err.Error())
		panic(err)
	}
	log.Printf("exe id is %s", exeId)
	resp, er := cli.ContainerExecAttach(ctx, exeId.ID, types.ExecStartCheck{})
	if er != nil {
		fmt.Println(er.Error())
		panic(err)
	}
	err = cli.ContainerExecStart(ctx, exeId.ID, types.ExecStartCheck{})
	if err != nil {
		log.Printf("exec error,err is %s", err.Error())
	}
	_, _ = stdcopy.StdCopy(os.Stdout, os.Stdout, resp.Reader)

	resp.Close()
	return nil
}

//往容器里复制文件
func (ce *ContainerEntity) CopyFileToContainer(ctx context.Context, srcFile string, dstPath string) {
	var content io.Reader
	targetFile := fmt.Sprintf("%s.tar", srcFile)
	_ = utils.TarFile(srcFile, targetFile)
	content, _ = os.Open(targetFile)
	err := cli.CopyToContainer(ctx, ce.contId, dstPath, content, types.CopyToContainerOptions{AllowOverwriteDirWithFile: true})
	if err != nil {
		panic(err.Error())
	}
}

//从容器里拷贝出文件
func (ce *ContainerEntity) CopyFileFromContainer(ctx context.Context, srcFile string, dstPath string) {
	readCloser, containerPathStatus, err := cli.CopyFromContainer(ctx, ce.contId, srcFile)
	defer readCloser.Close()
	if err != nil {
		panic(err.Error())
	}
	log.Printf("reusult json status %+v", containerPathStatus)
	buf := new(bytes.Buffer)
	_, _ = buf.ReadFrom(readCloser)
	tmpTarFile := fmt.Sprintf("/tmp/result_%s.tar", ce.randStr)
	err = ioutil.WriteFile(tmpTarFile, buf.Bytes(), 0666)
	defer os.Remove(tmpTarFile)
	_ = utils.Untar(tmpTarFile, dstPath)
	if err != nil {
		panic(err)
	}
}

//将相关文件传入docker内,构建运行环境
func (ce *ContainerEntity) BuildEnvAndRun(ctx context.Context, args *json_def.CompileAndRunArgs) {
	//创建临时目录
	absolutePath := fmt.Sprintf("%s/%s", DOCKERWORKPATH, ce.randStr)
	mkdirStr := fmt.Sprintf("mkdir -p %s", absolutePath)
	_ = ce.RunCmdInContainer(ctx, mkdirStr)

	//将文件拷贝到docker内去
	ce.CopyFileToContainer(ctx, args.SourceFile, absolutePath)
	ce.CopyFileToContainer(ctx, args.SysInputFile, absolutePath)
	ce.CopyFileToContainer(ctx, args.SysOutputFile, absolutePath)

	//执行相关命令
	chmodStr := fmt.Sprintf("chown root:root -R %s", absolutePath)

	//将dockerExe以及config目录放到absolute目录下
	copyExeAndConfigStr := fmt.Sprintf("cp %s %s && cp -r %s %s", DOCKEREXENAME, absolutePath, "config", absolutePath)
	_ = ce.RunCmdInContainer(ctx, copyExeAndConfigStr)

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
	exeRunnerStr := fmt.Sprintf("%s %s %s %d %d %d %s %s %s %s %s",
		executableName, args.Language, usrSourceFileInDocker,
		args.Time, args.Memory, args.Disk,
		userExeName, sysInputFileInDocker, sysOutputFileInDocker, userOutputFile,
		resultJsonFileInDocker)
	log.Printf("runner Str:%s\n", exeRunnerStr)
	unionChmodAndExeRunner := fmt.Sprintf("%s && %s", chmodStr, exeRunnerStr)
	_ = ce.RunCmdInContainer(ctx, unionChmodAndExeRunner)

	//获取docker内部的json文件
	//resultJsonName := fmt.Sprintf("./test/result_%s.json", utils.GenRandomStr(10))
	ce.CopyFileFromContainer(ctx, resultJsonFileInDocker, args.ResultJsonFile)
}
