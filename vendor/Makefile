# manage builds for these db engines
DIR=`realpath ../../`
PGVER=postgresql-16.1
MAVER=mariadb-10.11.2
#MYVER=mysql-8.2.0-linux-glibc2.28-x86_64
MYVER=mysql-8.0.35


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


postgres-libs:
	cp $(PGVER)/src/common/libpgcommon.a ../lib/
	cp $(PGVER)/src/fe_utils/libpgfeutils.a ../lib/
	cp $(PGVER)/src/interfaces/libpq/libpq.a ../lib/
	cp $(PGVER)/src/port/libpgport.a ../lib/


mysql-libs:
	cp mysql-8.2.0-linux-glibc2.28-x86_64/lib/libmysqlclient.a ../lib/
	cp mysql-8.2.0-linux-glibc2.28-x86_64/lib/libmysqlservices.a ../lib/


# Builds and installs to whatever the directory is above of this one
# May need to disable all of that connect stuff (mongo, and other other dbs that this can talk to)
#			-DCMAKE_INSTALL_PREFIX=$(DIR)
maria:
	-test ! -d $(MAVER) && tar xzf $(MAVER).tar.gz
	cd $(MAVER) && \
		cmake . \
			-DMYSQL_MAINTAINER_MODE=OFF \
			-DCONNECT_WITH_BSON=OFF \
			-DCONNECT_WITH_JDBC=OFF \
			-DCONNECT_WITH_LIBXML2=OFF \
			-DCONNECT_WITH_MONGO=OFF \
			-DCONNECT_WITH_ODBC=OFF \
			-DCONNECT_WITH_REST=OFF \
			-DCONNECT_WITH_VCT=OFF \
			-DCONNECT_WITH_ZIP=OFF \
			-DDISABLE_SHARED=OFF \
			-DCOMPRESS_DEBUG_SECTIONS=OFF \
			-DREPRODUCIBLE_BUILD=ON \
			-DCMAKE_INSTALL_PREFIX=../../ \
			-DMYSQL_DATADIR=$(DIR)/data && \
		make -j2 && \
		make install


mysql:
	-test ! -d $(MYVER) && tar xzf $(MYVER).tar.gz
	cd $(MYVER) && \
		cmake . \
			-DMYSQL_MAINTAINER_MODE=OFF \
			-DCONNECT_WITH_BSON=OFF \
			-DCONNECT_WITH_JDBC=OFF \
			-DCONNECT_WITH_LIBXML2=OFF \
			-DCONNECT_WITH_MONGO=OFF \
			-DCONNECT_WITH_ODBC=OFF \
			-DCONNECT_WITH_REST=OFF \
			-DCONNECT_WITH_VCT=OFF \
			-DCONNECT_WITH_ZIP=OFF \
			-DDISABLE_SHARED=OFF \
			-DCOMPRESS_DEBUG_SECTIONS=OFF \
			-DFORCE_INSOURCE_BUILD=1 \
			-DREPRODUCIBLE_BUILD=ON \
			-DCMAKE_INSTALL_PREFIX=../../ \
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
