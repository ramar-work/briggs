# Try loading from one to the other w/ a custom delimiter
PGUSER=postgres
TESTUSER=schtest
TESTPWD=schpass
PGDB=schdb
MYSQLUSER=root
MSSQLUSER=
ORACLEUSER=

x:
	../briggs -D mysql://schuser:schpass@localhost/schdb.scholarships --sql --root scholarships


postgres:
	psql -U $(PGUSER) < scholarships-postgres.sql


# Consider a PG_PASS export here
postgres-test:
	psql --csv -U $(TESTUSER) -c 'SELECT * FROM scholarships' $(PGDB) 


mysql:
	mysql -u $(MYSQLUSER) < scholarships-mysql.sql


mysql-test:
	mysql -u $(TESTUSER) --password=$(TESTPWD) -D schdb -e 'SELECT * FROM scholarships'



