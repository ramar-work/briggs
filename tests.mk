# Where is briggs in relation to this file?
EXECDIR=.

# A source user and password
SRCUSER="schtest"
SRCPASS="schpass"

# Destination user and password
DESTUSER="root"
DESTPASS=
#WAIT=read; clear
WAIT=
S="SUCCESS"
F="FAIL"
SILENT=2>/dev/null
SILENT=
STATUS=



# headers - Test the generation of headers from file, database table or SQL query
headers:
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests_hash_delimited.csv --headers; read  # Test a regular file
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv --headers; read  # Test a regular file
	$(EXECDIR)/briggs -d '#' -i $(EXECDIR)/tests/type_tests_hash_delimited.csv --headers; read  # Test a regular file
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --headers; read # Test a Postgres Connection
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --headers; read # Test a MySQL connection



# schema - Test the approximation of a database (or query) schema from file, database table or SQL query
schema:
	# Should always fail because there is no table name
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv --schema $(SILENT) && echo $(F) || echo $(S); $(WAIT)
	# Test regular CSV
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for postgres --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -o "postgres://postgres@localhost/washdb.washburn" --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -o "postgres://bomb:giantbomb@localhost/washdb" -T washburn --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -T type_tests --for mysql --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -o "mysql://root@localhost/washdb" -T washburn --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests.csv -o "mysql://bomb:giantbomb@localhost/washdb.washburn" --schema $(SILENT) && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb" --schema && echo $(F) || echo $(S); $(WAIT)
	# Test other delimited CSV
	# Test MySQL input data source
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb" --table washburn --schema && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --schema --for postgres && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" --schema --coerce "award_amount=integer" && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "mysql://bomb:giantbomb@localhost/washdb.washburn" -o "postgres://bomb:giantbomb@localhost/washdb" -T washburn --schema && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb" --schema && echo $(F) || echo $(S); $(WAIT)
	# Test Postgres input data source
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb" --table washburn --schema && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --schema && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --schema --for postgres && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" --schema --coerce "award_amount=integer" && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i "postgres://bomb:giantbomb@localhost/washdb.washburn" -o "postgres://bomb:giantbomb@localhost/washdb" -T washburn --schema && echo $(S) || echo $(F); $(WAIT)


# conversion - Test capability of translating from one format to another (TODO: Get code of common validator programs vs the actual output)
#conversion: load_media_tests
conversion:
	# To XML from flat file 
	$(EXECDIR)/briggs -n -i $(EXECDIR)/tests/type_tests.csv -T type_tests --xml | xmllint - && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -n -i $(EXECDIR)/tests/available_scholarships.csv -d ';' -T type_tests --xml | xmllint - && echo $(S) || echo $(F); $(WAIT)
	# To JSON from flat file
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/type_tests_postgres_small.csv -d '#' -T type_tests --json | jq && echo $(S) || echo $(F); $(WAIT)
	$(EXECDIR)/briggs -i $(EXECDIR)/tests/available_scholarships.csv -d ';' -T type_tests --json | jq && echo $(S) || echo $(F); $(WAIT)

boom:
	# To Postgres from flat file (the load commands won't be failures, but load_media_tests ought to catch that first)
	psql -U postgres -c 'DROP DATABASE IF EXISTS bixby';
	psql -U postgres -c 'CREATE DATABASE bixby';
	$(EXECDIR)/briggs --convert -d '#' \
		-i $(EXECDIR)/tests/type_tests_postgres_small.csv \
		-o "postgres://postgres@localhost/bixby.bixby_class"
	psql --expanded -U postgres -d bixby -c 'select * from bixby_class'; # There should be at least 3 lines
	psql -U postgres -c 'DROP DATABASE bixby';
	# To MySQL from flat file
	mysql -u root -e 'DROP DATABASE IF EXISTS bixby'
	mysql -u root -e 'CREATE DATABASE bixby'
	$(EXECDIR)/briggs --convert --coerce blobv=blob -d '#' \
		-i $(EXECDIR)/tests/type_tests_postgres_small.csv \
		-o "mysql://root@localhost/bixby.bixby_class"
	mysql --vertical -u root -D bixby -e 'SELECT * FROM bixby_class'; # There should be at least 3 lines
	mysql -u root -e 'DROP DATABASE bixby'
	# To MySQL from MySQL
	mysql -u root -e 'DROP DATABASE IF EXISTS bixby'
	mysql -u root -e 'CREATE DATABASE bixby'
	$(EXECDIR)/briggs --convert \
		-i "mysql://root@localhost/mysql_briggs_media_tests.mysql_fancy_media" \
		-o "mysql://root@localhost/bixby.bixby_class"
	mysql --vertical -u root -D bixby -e 'SELECT * FROM bixby_class'; # There should be at least 3 lines
	mysql -u root -e 'DROP DATABASE bixby'
	# To Postgres from Postgres
	psql -U postgres -c 'DROP DATABASE IF EXISTS bixby';
	psql -U postgres -c 'CREATE DATABASE bixby';
	$(EXECDIR)/briggs -i "postgres://postgres@localhost/pg_briggs_media_tests.postgres_fancy_media" \
		-o "postgres://postgres@localhost/bixby.bixby_class" --convert
	psql --expanded -U postgres -d bixby -c 'select * from bixby_class'; # There should be at least 3 lines
	psql -U postgres -c 'DROP DATABASE bixby';
	# To MySQL from Postgres
	mysql -u root -e 'DROP DATABASE IF EXISTS bixby'
	mysql -u root -e 'CREATE DATABASE bixby'
	$(EXECDIR)/briggs --convert --coerce booleanv=char \
		-i "postgres://postgres@localhost/pg_briggs_media_tests.postgres_fancy_media" \
		-o "mysql://root@localhost/bixby.bixby_class"
	mysql --vertical -u root -D bixby -e 'SELECT * FROM bixby_class';
	mysql -u root -e 'DROP DATABASE bixby'
	# To Postgres from MySQL 
	psql -U postgres -c 'DROP DATABASE IF EXISTS bixby';
	psql -U postgres -c 'CREATE DATABASE bixby';
	$(EXECDIR)/briggs --convert --coerce timev=time,yearv=integer,textv=text,blobv=text \
		-i "mysql://root@localhost/mysql_briggs_media_tests.mysql_fancy_media" \
		-o "postgres://postgres@localhost/bixby.bixby_class"
	psql --expanded -U postgres -d bixby -c 'select * from bixby_class';# There should be at least 3 lines
	psql -U postgres -c 'DROP DATABASE bixby';



# load_media_tests - Load all relevant media tests to database
load_media_tests:
	mysql -u root < tests/media_mysql.sql
	postgres -U postgres < tests/media_postgres.sql



# parsing - Test parsing of different data source names (really should only work with DEBUG_H) 
# TODO: Need WAAAAAAAY more tests than this
parsing:
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@hell.to.the.naw.com/washdb.washburn" -X
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@localhost/washdb.washburn" -X
	$(EXECDIR)/briggs -i "postgresql://bomb:giantbomb@hell.to.the.naw.com/washdb.washburn?x=a&asdfsfd8lasdfas.asdfsafd=asdfsadfsasd8234324nn23" -X



.PHONY: headers schema
