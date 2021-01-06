package context

import (
	"github.com/jinzhu/gorm"
	_ "github.com/jinzhu/gorm/dialects/mysql"
	_ "github.com/jinzhu/gorm/dialects/sqlite"
	"laser-control/logger"
	"time"
)

type Context struct {
	config *Config
	logger *logger.Logger
	db     *gorm.DB
}

func New() *Context {
	config := NewConfig()
	context := &Context{
		config: config,
		logger: logger.New(),
	}

	return context
}

func (context *Context) GetConfig() *Config {
	return context.config
}

func (context *Context) Log() *logger.Logger {
	return context.logger
}

func (context *Context) connectToDatabase() error {
	dbDriver := context.GetConfig().DatabaseDriver()
	dbDsn := context.GetConfig().DatabaseDsn()

	db, err := gorm.Open(dbDriver, dbDsn)
	if err != nil || db == nil {
		for i := 1; i <= 12; i++ {
			time.Sleep(time.Second)
			db, err = gorm.Open(dbDriver, dbDsn)
			if db != nil && err == nil {
				break
			}
			context.logger.Panic(err.Error())
		}

		if err != nil || db == nil {
			context.logger.Panic(err.Error())
		}
	}
	context.db = db
	return err
}

func (context *Context) Db() *gorm.DB {
	if context.db == nil {
		context.connectToDatabase()
	}

	if context.db != nil && context.GetConfig().Debug() {
		context.db.LogMode(true)
	}
	return context.db
}
