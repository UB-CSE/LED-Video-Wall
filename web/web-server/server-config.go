package main

import (
	"log"
	"os"

	"github.com/joho/godotenv"
)

type ServerConfig struct {
	VWServerPath           string
	LogsDir                string
	MatrixConfigsDir       string
	UserCanvasConfigsDir   string
	UserResourceUploadsDir string
	DisableGPU             bool
	DebugMode              bool
}

func GetServerConfig() *ServerConfig {
	_ = godotenv.Load()

	serverConfig := &ServerConfig{}

	ok := true

	serverConfig.VWServerPath, ok = os.LookupEnv("LEDVW_SERVER_EXE_PATH")
	if !ok {
		log.Fatal("environment variable LEDVW_SERVER_EXE_PATH not set")
	}
	serverConfig.LogsDir, ok = os.LookupEnv("LEDVW_LOGS_DIR")
	if !ok {
		log.Fatal("environment variable LEDVW_LOGS_DIR not set")
	}
	serverConfig.MatrixConfigsDir, ok = os.LookupEnv("LEDVW_MATRIX_CONFIGS_DIR")
	if !ok {
		log.Fatal("environment variable LEDVW_MATRIX_CONFIGS_DIR not set")
	}
	serverConfig.UserCanvasConfigsDir, ok = os.LookupEnv("LEDVW_USER_CANVAS_CONFIGS_DIR")
	if !ok {
		log.Fatal("environment variable LEDVW_USER_CANVAS_CONFIGS_DIR not set")
	}
	serverConfig.UserResourceUploadsDir, ok = os.LookupEnv("LEDVW_USER_RESOURCE_UPLOADS_DIR")
	if !ok {
		log.Fatal("environment variable LEDVW_USER_RESOURCE_UPLOADS_DIR not set")
	}
	serverConfig.DisableGPU = os.Getenv("LEDVW_DISABLE_GPU") == "1"
	serverConfig.DebugMode = os.Getenv("LEDVW_DEBUG") == "1"

	return serverConfig
}
