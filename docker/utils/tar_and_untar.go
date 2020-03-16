package utils

import (
	"archive/tar"
	"fmt"
	"io"
	"math/rand"
	"os"
	"time"
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

func Untar(tarFile, destFile string) error {
	var srcFile = tarFile
	fr, err := os.Open(srcFile)
	if err != nil {
		panic(err)
	}
	defer fr.Close()
	tr := tar.NewReader(fr)

	for hdr, err := tr.Next(); err != io.EOF; hdr, err = tr.Next() {
		if err != nil {
			panic(err)
		}
		fi := hdr.FileInfo()
		fw, err := os.Create(destFile)
		if err != nil {
			panic(err)
		}
		if _, err = io.Copy(fw, tr); err != nil {
			panic(err)
		}
		_ = os.Chmod(destFile, fi.Mode().Perm())
		_ = fw.Close()
	}
	return nil
}

func GenRandomStr(l int) string {
	str := "0123456789abcdefghijklmnopqrstuvwxyz"
	bytes := []byte(str)
	var result []byte
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < l; i++ {
		result = append(result, bytes[r.Intn(len(bytes))])
	}
	return string(result)
}
