briggs
======

Briggs is a pretty simple C program for converting test data into many popular 
formats, such as

- JSON
- SQLite
- CSV
- MySQL
- SQL Server
- Java classes
- C / C++ structures
- ColdFusion
- and hopefully many more... 

Let's go champ...




Installation
------------

`make && make install` should be all that's needed to run on your system.




Usage
-----

Options for briggs are as follows:

<ul>
<li>-c, --convert &lt;arg&gt;    Load a file (should be a CSV now)</li>
<li>-d, --delimiter &lt;arg&gt;  Specify a delimiter</li>
<li>-j, --json                   Convert into JSON.</li>
<li>-x, --xml                    Convert into XML.</li>
<li>-p, --prefix &lt;arg&gt;     Specify a prefix</li>
<li>-s, --suffix &lt;arg&gt;     Specify a suffix</li>
</ul>

The repository ships with a file called `test/full-tab.csv`; a semicolon-seperated document containing a list of scholarships.  To convert this data into JSON, run:

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

Data that needs additional padding (like XML, usually) can make use of the --prefix and --suffix options.  For example, let's re-run our previous command, but convert the data to XML:

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
