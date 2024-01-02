# manage builds for these db engines
DIR=`realpath ../../`
PGVER=postgresql-16.1
MYVER=mariadb-10.11.2 

xx:
	echo $(DIR)


# postgres - Builds and installs to whatever the directory is above of this one
# For both servers (since we're just testing) it might make sense to use a regular user
postgres:
	-test ! -d $(PGVER) && tar xzf $(PGVER).tar.gz
	cd $(PGVER) && \
		export MAKELEVEL=0 && \
		./configure --prefix=$(DIR) && \
		make -j2 && \
		make install


# Builds and installs to whatever the directory is above of this one
# May need to disable all of that connect stuff (mongo, and other other dbs that this can talk to)
#			-DCMAKE_INSTALL_PREFIX=$(DIR)
mysql:
	-test ! -d $(MYVER) && tar xzf $(MYVER)
	cd $(MYVER) && \
		cmake . \
			-DMYSQL_MAINTAINER_MODE=OFF \
			-DCONNECT_WITH_BSON=OFF\
			-DCONNECT_WITH_JDBC=OFF\
			-DCONNECT_WITH_LIBXML2=OFF\
			-DCONNECT_WITH_MONGO=OFF\
			-DCONNECT_WITH_ODBC=OFF\
			-DCONNECT_WITH_REST=OFF\
			-DCONNECT_WITH_VCT=OFF\
			-DCONNECT_WITH_ZIP=OFF\
			-DCMAKE_INSTALL_PREFIX=../../ \
			-DWITHOUT_SERVER=ON \
			-DMYSQL_DATADIR=$(DIR)/data && \
		make -j2 && \
		make install


no:
	cmake --build -j 2 && \
	cmake --install


postgres-server:
	printf ''

mysql-server:
	printf ''
