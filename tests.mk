# Where is briggs in relation to this file?
EXECDIR=.

# A source user and password
SRCUSER="schtest"
SRCPASS="schpass"

# Destination user and password
DESTUSER="root"
DESTPASS=


mysql_mysql:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_oracle:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_postgres:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_sqlite3:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_sqlserver:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_mongodb:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_db2:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_mysql:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_oracle:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_postgres:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_sqlite3:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_sqlserver:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_mongodb:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_db2:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_mysql:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_oracle:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_postgres:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_sqlite3:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_sqlserver:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_mongodb:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_db2:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_mysql:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_oracle:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_postgres:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_sqlite3:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_sqlserver:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_mongodb:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_db2:
	$(EXECDIR)/briggs -i "sqlite3://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_mysql:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_oracle:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_postgres:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_sqlite3:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_sqlserver:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_mongodb:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlserver_db2:
	$(EXECDIR)/briggs -i "sqlserver://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_mysql:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_oracle:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_postgres:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_sqlite3:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_sqlserver:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_mongodb:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mongodb_db2:
	$(EXECDIR)/briggs -i "mongodb://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_mysql:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_oracle:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_postgres:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_sqlite3:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_sqlserver:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_mongodb:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

db2_db2:
	$(EXECDIR)/briggs -i "db2://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

