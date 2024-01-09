# Where is briggs in relation to this file?
EXECDIR=.

# A source user and password
SRCUSER="schtest"
SRCPASS="schpass"

# Destination user and password
DESTUSER="root"
DESTPASS=


# Some targets for command line testing
headers:
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv --headers  # Test a regular file
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --headers # Test a Postgres Connection
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --headers # Test a MySQL connection

schema:
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --schema
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for postgres --schema
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for postgres --coerce blobv=bytea --schema
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for mysql --coerce blobv=BLOB --schema


xx:
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for mysql --schema
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --schema
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --schema
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --schema

non:
	psql -U postgres < tests/washburn_postgres.sql
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" -o "mysql://root@localhost/$(DBNAME).washburn" --convert
	


# Test longer hostnames
ncat_test0:
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@hell.to.the.naw.com/washdb.washburn" -X
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@localhost/washdb.washburn" -X
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@hell.to.the.naw.com/washdb.washburn?x=a&asdfsfd8lasdfas.asdfsafd=asdfsadfsasd8234324nn23" -X


# Test #1
ncat_test1: DBNAME=washdb_postgres_ncat_test_1
ncat_test1:
	mysql -u root < tests/washburn_mysql.sql
	psql -U postgres -c 'DROP DATABASE IF EXISTS $(DBNAME)'
	psql -U postgres -c 'CREATE DATABASE $(DBNAME)'
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" -o "postgres://postgres@localhost/$(DBNAME).washburn" --convert
	psql -U postgres --dbname=$(DBNAME) -c 'select * from washburn';


# Test #2
ncat_test2: DBNAME=washdb_postgres_ncat_test_2
ncat_test2:
	psql -U postgres < tests/washburn_postgres.sql
	mysql -u root -e 'DROP DATABASE IF EXISTS $(DBNAME)' 	
	mysql -u root -e 'CREATE DATABASE $(DBNAME)' 	
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" -o "mysql://root@localhost/$(DBNAME).washburn" --convert
	mysql -u root --database=$(DBNAME) -e 'select * from washburn';


# Test #3
ncat_test3: DBNAME=washdb_postgres_ncat_test_3
ncat_test3:
	mysql -u root < tests/washburn_mysql.sql
	mysql -u root -e 'DROP DATABASE IF EXISTS $(DBNAME)' 	
	mysql -u root -e 'CREATE DATABASE $(DBNAME)' 	
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" -o "mysql://root@localhost/$(DBNAME).washburn" --convert
	mysql -u root --database=$(DBNAME) -e 'select * from washburn';


# Test #4
ncat_test4: DBNAME=washdb_postgres_ncat_test_4
ncat_test4:
	psql -U postgres < tests/washburn_postgres.sql
	psql -U postgres -c 'DROP DATABASE IF EXISTS $(DBNAME)'
	psql -U postgres -c 'CREATE DATABASE $(DBNAME)'
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" -o "postgres://postgres@localhost/$(DBNAME).washburn" --convert
	psql -U postgres --dbname=$(DBNAME) -c 'select * from washburn';


dump_ncat_test1: TABLE=washdb_postgres_ncat_test_1
dump_ncat_test1:
	psql -U postgres --dbname=$(TABLE) -c 'select * from washburn';


dump_ncat_test2: TABLE=washdb_postgres_ncat_test_2
dump_ncat_test2:
	mysql -u root --database=$(TABLE) -e 'select * from washburn';



mysql_mysql:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_oracle:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

# Test #1
mysql_postgres:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

mysql_sqlite3:
	$(EXECDIR)/briggs -i "mysql://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://schdb.scholarships" --convert

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
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://schdb.scholarships" --convert

oracle_sqlserver:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_mongodb:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

oracle_db2:
	$(EXECDIR)/briggs -i "oracle://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

# Test #2
postgres_mysql:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_oracle:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_postgres:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_sqlite3:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlite3://schdb.scholarships" --convert

postgres_sqlserver:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_mongodb:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

postgres_db2:
	$(EXECDIR)/briggs -i "postgres://$(SRCUSER):$(SRCPASS)@localhost/schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_mysql:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "mysql://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_oracle:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "oracle://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_postgres:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "postgres://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_sqlite3:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "sqlite3://schdb.scholarships" --convert

sqlite3_sqlserver:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "sqlserver://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_mongodb:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "mongodb://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

sqlite3_db2:
	$(EXECDIR)/briggs -i "sqlite3://schdb.scholarships" -o "db2://$(DESTUSER):$(DESTPASS)@localhost/schdb.scholarships" --convert

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

.PHONY: headers schema
