<html>

briggs
======

Briggs is a simple C program for converting test data between many popular
formats, such as:

- JSON
- CSV
- XML
- C / C++ structures
- SQL
- and hopefully many more... 

Let's go champ...




Installation
------------

<code>make && make install</code> should be all that's needed to run on your system.

<pre>
-c, --convert &lt;arg&gt;         Convert a supplied CSV file &lt;arg&gt; to another format
-e, --headers &lt;arg&gt;         Only display the headers in &lt;arg&gt;
-d, --delimiter &lt;arg&gt;       Specify a delimiter           
-r, --root &lt;arg&gt;            Specify a "root" name for certain types of structures.
  , --no-unsigned           Remove any unsigned character sequences.
-u, --output-delimiter &lt;arg&gt;       Specify an output delimiter for strings
  , --comma                 Convert into XML.             
  , --cstruct               Convert into a C struct.      
  , --carray                Convert into a C-style array. 
-j, --json                  Convert into JSON.            
-x, --xml                   Convert into XML.             
-i, --insert-newline        Generate newline after each row.
-p, --prefix &lt;arg&gt;          Specify a prefix              
-s, --suffix &lt;arg&gt;          Specify a suffix              
-h, --help                  Show help.                    
</pre>


Usage
-----

Options | Description
------- | -------------
-c, --convert &lt;arg&gt;   | Load a file (should be a CSV now)
-d, --delimiter &lt;arg&gt; | Specify a delimiter
-j, --json                  | Convert into JSON.
-x, --xml                   | Convert into XML.
-p, --prefix &lt;arg&gt;    | Specify a prefix
-s, --suffix &lt;arg&gt;    | Specify a suffix


<br />
The repository ships with a file called <code>test/full-tab.csv</code>; a semicolon-seperated document containing a list of scholarships.  To convert this data into JSON, run:

<pre>
briggs --convert test/full-tab.csv --json --delimiter ';'
</pre>

briggs will automatically use the first line of your data as key names.  So we should see something similar to the following in a terminal.

<pre>
{
	"Date": $val
	"Name": $val
	"Award Amount": $val
	"Address": $val
	...
}
</pre> 

Data that needs additional padding (like XML, usually) can make use of the <code>--prefix</code> and <code>--suffix</code> options.  For example, let's re-run our previous command, but convert the data to XML:

<pre>
briggs --convert test/full-tab.csv --xml --delimiter ';' --prefix '<Node>' --suffix '</Node>'
</pre>

Now, we'll get:
<pre>
&lt;Node&gt;
	&lt;Date&gt; $val &lt;/Date&gt; 
	&lt;Name&gt; $val &lt;/Name&gt; 
	&lt;Award Amount&gt; $val &lt;/Award Amount&gt; 
	&lt;Address&gt; $val &lt;/Address&gt; 
	...
&lt;/Node&gt;
</pre>



Help & Comments
---------------
This is open-source software, and a project currently in beta.

Please submit a pull request, or just email me directly at ramar@tubularmodular.com if you're having issues using this.



<style>
html {
	min-width: 330px;
	width: 50%;
	margin: 0 auto;
	padding-bottom: 100px;
}
h2 {
	margin-top: 40px;
}
p {
	margin: 0 auto;
}
table tr:nth-child(1) {
	text-align: left;
}
table tr:nth-child(n+1) {
	font-family: monospace;	
}
code {
	background-color: black;
	color: white;
	font-family: monospace;	
	padding: 2px;
	letter-spacing: 1px;
}
pre:nth-of-type(n+2) {
	background-color: black;
	color: white;
	padding: 15px;
	padding-top: 15px;
	padding-bottom: 15px;
}
</style>
</html>
