package logger

import (
	"github.com/spf13/viper"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	lumberjack "gopkg.in/natefinch/lumberjack.v2"
	"os"
	"time"
)

type Field = zapcore.Field
type ObjectEncoder = zapcore.ObjectEncoder
type Logger = zap.Logger

func New() *Logger {
	hook := lumberjack.Logger{
		Filename:   viper.GetString("logger.fileName"),
		MaxSize:    viper.GetInt("maxSize"),
		MaxBackups: viper.GetInt("maxBackups"),
		MaxAge:     viper.GetInt("maxAge"),
		Compress:   viper.GetBool("compress"),
	}

	fileWriter := zapcore.AddSync(&hook)
	consoleDebugging := zapcore.Lock(os.Stdout)
	var level zapcore.Level
	switch viper.GetString("logger.level") {
	case "debug":
		level = zapcore.DebugLevel
	case "info":
		level = zapcore.InfoLevel
	case "error":
		level = zapcore.ErrorLevel
	default:
		level = zapcore.InfoLevel
	}

	isDebug := viper.GetBool("debug")

	var encoder zapcore.Encoder
	var allCore []zapcore.Core
	if isDebug {
		encoder = zapcore.NewConsoleEncoder(zap.NewDevelopmentEncoderConfig())
	} else {
		encoder = zapcore.NewConsoleEncoder(zap.NewProductionEncoderConfig())
	}

	if isDebug {
		allCore = append(allCore, zapcore.NewCore(encoder, consoleDebugging, level))
	}
	allCore = append(allCore, zapcore.NewCore(encoder, fileWriter, level))
	core := zapcore.NewTee(allCore...)

	logger := zap.New(core).WithOptions(zap.AddCaller())
	return logger
}

func String(k, v string) Field {
	return zap.String(k, v)
}

func Duration(k string, d time.Duration) Field {
	return zap.Duration(k, d)
}

func Float64(key string, val float64) Field {
	return zap.Float64(key, val)
}

func Time(key string, val time.Time) Field {
	return zap.Time(key, val)
}

func Int(k string, i int) Field {
	return zap.Int(k, i)
}

func Array(key string, val zapcore.ArrayMarshaler) Field {
	return zap.Array(key, val)
}

func Int64(k string, i int64) Field {
	return zap.Int64(k, i)
}

func Error(v error) Field {
	return zap.Error(v)
}

func Object(key string, val zapcore.ObjectMarshaler) Field {
	return zap.Object(key, val)
}
