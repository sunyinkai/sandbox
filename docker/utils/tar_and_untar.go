package utils

import (
	"archive/tar"
	"fmt"
	"io"
	"os"
)

func TarFile(srcFile string, dstFile string) error {
	fw, err := os.Create(dstFile)
	if err != nil {
		panic(err)
	}
	defer fw.Close()
	tw := tar.NewWriter(fw)
	defer tw.Close()

	fileInfo, err := os.Stat(srcFile)
	if err != nil {
		panic(err)
	}
	header, err := tar.FileInfoHeader(fileInfo, "")
	err = tw.WriteHeader(header)
	if err != nil {
		panic(err)
	}

	fr, err := os.Open(srcFile)
	if err != nil {
		panic(err)
	}
	defer fr.Close()
	bytesNum, err := io.Copy(tw, fr)
	if err != nil {
		panic(err)
	}
	fmt.Println("write bytes Num", bytesNum)
	return nil
}
