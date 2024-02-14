briggs
======
`briggs` is a simple command-line program for converting test data between many popular
formats, such as:

- JSON
- CSV
- XML
- C/C++ & Java structures
- SQL
- and hopefully many more... 

Let's go champ...




Installation
------------

To get `briggs` working, you'll need at a minimum:

- OpenSSL

If you want MySQL, Postgres or Oracle support you'll need to add those libraries to your system or request that they be compiled in if building `briggs` from source.

Keep reading for instructions on how to build.


### Yum-Based Distributions (Red Hat, Oracle, Fedora, etc.)

yum install postgresql


### Apt-Based Distributions

postgresql


### Building from Source

#### 3-step install

On Unix-like systems (e.g. not Windows) `make && make install` should be all 
that's needed to run on your system.  Windows users can get up and running fairly
easily via Cygwin.  

Provided you have wget or curl on your system, run the following three commands
to get `briggs` built and installed on your system.

<pre>
$ wget https://github.com/zaiah-dj/briggs/archive/master.zip
$ unzip master.zip && cd briggs-master/ && make
$ sudo make install 
</pre>



Usage
-----
Options for running `briggs` are as follows.

<pre>
-i, --input &lt;arg&gt;             Specify an input datasource (required)
    --create                  If an output source does not exist, create it
-o, --output &lt;arg&gt;            Specify an output datasource (optional, default is stdout)
-c, --convert                 Converts input data to another format or into a datasource
-H, --headers                 Displays headers of input datasource and stop
-S, --schema                  Generates an SQL schema using headers of input datasource
-d, --delimiter &lt;arg&gt;         Specify a delimiter when using a file as input datasource
-u, --output-delimiter &lt;arg&gt;  Specify an output delimiter when generating serialized output
-t, --typesafe                Enforce and/or enable typesafety
-C, --coerce                  Specify a type for a column   
-f, --format &lt;arg&gt;            Specify a format to convert to
-j, --json                    Convert into JSON (short for --convert --format "json")
-x, --xml                     Convert into XML (short for --convert --format "xml")
-q, --sql                     Convert into general SQL INSERT statement.
-p, --prefix &lt;arg&gt;            Specify a prefix              
-s, --suffix &lt;arg&gt;            Specify a suffix              
    --class &lt;arg&gt;             Generate a Java-like class using headers of input datasource
    --struct &lt;arg&gt;            Generate a C-style struct using headers of input datasource
    --for &lt;arg&gt;               Use &lt;arg&gt; as the backend when generating a schema
                              (See `man briggs` for available backends)
    --camel-case              Use camel case for class properties or struct labels
-n, --no-newline              Do not generate a newline after each row.
    --id &lt;arg&gt;                Add and specify a unique ID column named &lt;arg&gt;.
    --add-datestamps          Add columns for date created and date updated
-a, --ascii                   Remove any characters that aren't ASCII and reproduce
    --no-unsigned             Remove any unsigned character sequences.
-T, --table &lt;arg&gt;             Use the specified table when connecting to a database for source data
-Q, --query &lt;arg&gt;             Use this query to filter data from datasource
-y, --stats                   Dump stats at the end of an operation
-X, --dumpdsn                 Dump the DSN only. (DEBUG)    
-h, --help                    Show help.                    
</pre>


Quickstart
----------

The repository ships with a few test files in the test/ folder. 
Let's start off with the file called `test/inventory.csv`.  Running 
the following command:

<pre>
briggs --convert test/inventory.csv --stream json
</pre>

(or in short form:)

<pre>
briggs -c test/inventory.csv -j
</pre>

Will yield a list resembling the following:
<pre>
[{
	 "inv_cost": "0.99"
	,"inv_qty": "-1"
	,"inv_item": "2020-11-04-Sunrise.mp3"
	,"inv_image": ""
	,"inv_description": "Mr. Doctor – Sunrise (MP3 encoded)"
}
,{
	 "inv_cost": "0.99"
	,"inv_qty": "-1"
	,"inv_item": "2020-11-04-Scapes-v1.mp3"
	,"inv_image": ""
	,"inv_description": "Mr. Doctor – Scapes (MP3 encoded)"
}
/* this list continues... */
}]
</pre>

If we take a peek at that file, we'll see that `briggs` is automatically 
using the first line of data as key names. 

<pre>
//Output of head -n 1 test/inventory.csv
inv_cost,inv_qty,inv_item,inv_image,inv_description
</pre>

If we would like to see those in one column, or just need to see what the
key names will look like in a data structure, we can use the following to
see what `briggs` made of the file's headers:

<pre>
inv_cost
inv_qty
inv_item
inv_image
inv_description
</pre>

While not terribly useful on it's own, the --headers option can be used in
conjunction with `sed` or similar to do quick data formatting.  So with the
following:
<pre>
briggs --headers test/inventory.csv | sed 's/_/entory /g; s;\(.*\);&lt;h2&gt;\1&lt;/h2&gt;;'
</pre>

...we'll get:
<pre>
&lt;h2&gt;inventory cost&lt;/h2&gt;
&lt;h2&gt;inventory qty&lt;/h2&gt;
&lt;h2&gt;inventory item&lt;/h2&gt;
&lt;h2&gt;inventory image&lt;/h2&gt;
&lt;h2&gt;inventory description&lt;/h2&gt;
</pre>


### Conversion to SQL

Now, let's say we've been tasked with saving this data to a sqlite database of
some sort.   Assuming we're using something like sqlite, this should be a
fairly trivial operation.

First, if there is no table we'll have to create it.
<pre>
//create statement for the inventory table
CREATE TABLE inventory (
	inv_cost TEXT,
	inv_qty TEXT,
	inv_item TEXT,
	inv_image TEXT,
	inv_description TEXT
);
</pre>

Second, let `briggs` handle the conversion and place the insert 
statement into a file called inventory.sql.
<pre>
briggs -c test/inventory.csv -f sql --root inventory > inventory.sql
</pre>

This will yield:
<pre>
INSERT INTO inventory ( inv_cost,inv_qty,inv_item,inv_image,inv_description ) VALUES 
	( '0.99','-1','2020-11-04-Sunrise.mp3','','Mr. Doctor – Sunrise (MP3 encoded)' );
INSERT INTO inventory ( inv_cost,inv_qty,inv_item,inv_image,inv_description ) VALUES 
	( '0.99','-1','2020-11-04-Scapes-v1.mp3','','Mr. Doctor – Scapes (MP3 encoded)' );
/* .... more records .... */
INSERT INTO inventory ( inv_cost,inv_qty,inv_item,inv_image,inv_description ) VALUES 
	( '0.99','-1','2020-11-04-MoreLyrical.mp3','','Mr. Doctor – More Lyrical (MP3 encoded)' );
</pre>

Notice how we used the `--root` option to specify the name of the table.  If we 
didn't use it, `briggs` would have used the word 'root' as our assumed table name. 

It's worth noting that for options in which a named data structure will make 
sense, `briggs` will rely on the root flag.  So, for example, if we wanted a C 
structure with a particular variable name, the --root option would be used to 
specify that name.  Obviously, for certain formats this won't apply.


### Slightly Advanced

Let's take another example, using an Oracle database for storage.  By default,
`briggs` will output SQL insert code with a single-quote delimiter for strings.  
So what if we encounter a string like the third line below in one of our data 
files? (Leading spaces have been added for clarity.)

<pre>
10001, 100 E Santos St., Charlotte, NC, 28201, Fish populate this neighborhood.
10000, 20 E Flatbush Ave., Charlotte, NC, 28201, It is so nice here.
10009,777 Unlucky Cir.,Charlotte,NC,28201,This is sure to trip up ' ' 'anyone'   '.
10007, 80 E Yonkers Ct., Charlotte, NC, 28206, Runaway... fast.
</pre>

Running our usual insert generator will give us something that doesn't work 
quite so well with a default Oracle XE install:

<pre>
//Generate an INSERT statement and save to 'oracle.sql'
briggs --sql -c test/oracle.csv > oracle.sql
</pre>

<pre>
//Dump the contents of 'oracle.sql'
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( '10001','100 E Santos St.','Charlotte','NC','28201','Phish are all over this neighborhood.' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( '10000','20 E Flatbush Ave.','Charlotte','NC','28201','It is so nice here.' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( '10009','777 Unlucky Cir.','Charlotte','NC','28201','This is sure to trip up ' ' 'anyone'   '.' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( '10007','80 E Yonkers Ct.','Charlotte',' NC','28206','Runaway... fast.' );
</pre>

<pre>
sudo -Eu oracle sqlplus / as sysdba < oracle.sql

/* ...Oracle's diagnostic and version information would display here... */

SQL>   2  
1 row created.

SQL>   2  
1 row created.

SQL>   2  ERROR:
ORA-01756: quoted string not properly terminated

SQL>   2  
1 row created.
</pre>

Yep, that error message says it all.  The typical SQL database does not handle 
single quotes very well.  For what it's worth, Oracle handled this extremely 
well, as some database engines won't even tell you why this particular 
statement does not work.  

So, how do we fix it?  The first solution is to just escape the single quote
with either a backslash (\) or another single quote (').  So all the characters
in our offending field would look something like this:

<pre>
'This is sure to trip up \' \' \'anyone\'   \'.'
//or
'This is sure to trip up '' '' ''anyone''   ''.'
</pre>

This can work for this ONE change.  But what if we've got a thousand lines like
this?  Or even worse, what if multiple fields in our data look like this?

<pre>
//Yuck...
10001, 100 'E' Santos St., Charlotte, NC, 28201, Fish populate this neighborhood.
10000, 20 E Flatbush Ave., Charlotte, 'NC', 28201, It's so nice here on 'Tuesday'.
10009, 777 Unlucky Cir., Charlotte, NC, 28201, This is sure to trip up ''anyone''.
10007, 80 E Yonkers Ct., 'Charlotte', NC, 28206, Runaway... fast.
</pre>

The second solution will involve relying on a completely different delimiter.
Fortunately for us, Oracle has a built-in way to solve this problem via the 'q' 
tag.  It tells the database engine that it is about to encounter an unusual 
string, that may contain strange characters like the ones in our test file.   
The idea is similar to a heredoc for those familiar with Bash or PHP.  You can
read more about the 'q' mechanism <a href="https://docs.oracle.com/cd/B19306_01/server.102/b14200/sql_elements003.htm#sthref344">here</a>.

To make use of things like this, `briggs` comes with an `--output-delimiter` option 
(or `-u` for short) to change the default string enclosure.  It is invoked when 
converting to SQL and used as a delimiter when not specifying a particular format.

So, with no format specified, we'll get something like the following when running
`briggs` with that flag:
<pre>
//Command
briggs -c test/oracle.csv -u "q'[,]'"

//Yields
/*
record => q'[10001]'
address => q'[100 E Santos St.]'
city => q'[Charlotte]'
state => q'[NC]'
zip => q'[28201]'
description => q'[Phish are all over this neighborhood.]'

record => q'[10000]'
address => q'[20 E Flatbush Ave.]'
city => q'[Charlotte]'
state => q'[NC]'
zip => q'[28201]'
description => q'[It is so nice here.]'

record => q'[10009]'
address => q'[777 Unlucky Cir.]'
city => q'[Charlotte]'
state => q'[NC]'
zip => q'[28201]'
description => q'[This is sure to trip up ' ' 'anyone'   '.]'

record => q'[10007]'
address => q'[80 E Yonkers Ct.]'
city => q'[Charlotte]'
state => q'[NC]'
zip => q'[28206]'
description => q'[Runaway... fast.]'

*/
</pre>

And to solve our current problem, we can run:
<pre>
//Command
briggs --sql -c test/oracle.csv -u "q'[,]'" > oracle.sql
</pre>

...which will yield
<pre>
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( q'[10001]',q'[100 E Santos St.]',q'[Charlotte]',q'[NC]',q'[28201]',q'[Phish are all over this neighborhood.]' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( q'[10000]',q'[20 E Flatbush Ave.]',q'[Charlotte]',q'[NC]',q'[28201]',q'[It is so nice here.]' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( q'[10009]',q'[777 Unlucky Cir.]',q'[Charlotte]',q'[NC]',q'[28201]',q'[This is sure to trip up ' ' 'anyone'   '.]' );
INSERT INTO root ( record,address,city,state,zip,description ) VALUES 
	( q'[10007]',q'[80 E Yonkers Ct.]',q'[Charlotte]',q'[NC]',q'[28206]',q'[Runaway... fast.]' );
</pre>



Rationale
---------

One of the reasons for writing this tool was a need to stay away from in-depth
`sed` & `awk` incantations.  In theory, they can solve just about any problem 
one would encounter when dealing with ASCII text.  In practice (for me anyway), 
it always takes too many lines and far too much thought to get at a quick solution.





Help & Comments
---------------
This is open-source software, and a project currently in beta.

`briggs` was named in honor of New York's
Shannon Briggs -- notorious for his vicious (and hilarious) trolling of 
Vladimir Klitschko before the pair's promised title fight.

Please submit a pull request, a comment here on Github, or just email me 
directly at ramar@ramar.work if you're having issues using it.

