<html>
<center>
<pre>
K0kdc::::::::clllllllllllllllllllllcccccccccc:cllll
KKKK0kl::::::clllllllloxkOO0000OOkxdlllllllcccc;;;,
KKKKKKKkc::::llllllok0000000000000000Odlllllllc;;;,
KKKKKKKK0l:::l,,,',,,oO0KKK0000000000000xlllll:;;;;
KKKKKKKKK0;;'...........;lk0KKK0KK0000000Oolll:;;;;
KKKKKKKKdc...... ...........;OK0K0000000000oll:;;cO
OOK0KKkk......',,,;;,,,......cKKKKKK0000000Oll:;:OO
ddddddo:..,,;;;::::::;,'....  c00K0000K00000dl:;x00
kOOOOk'..,;,;::looool:;'''..   dOOkkkOkxOx00xl::O00
KKKKKK:.';,;;;:clllcc::;,'.....KK0000000KO00dl:;O00
KKKKK0: ',,;:cldddodo:,........k0K0KK0K00000ll:;d00
KKKKK0o.;,.....';co:,..'''.....;k0K00000000oll:::O0
KKKKK0o':..''.;,.;c;.,:,',.,,..'cK00000000olll:;;:x
KKKKko:,;;'',,,,':l:;;:;;::;'..;:0000000xlllll;;;;;
kdl::::;:;;:::l;;:cc::;lo:;'...';0000kdlllcccc;;;;;
cc:::::.;,';:oo;;;lc;,,::l;,....;xdolllllccccc;;;;,
kkkkkxxl,,'';cc;'.......',:;'....:cccccccccccc::::;
OOkkkkkd,,.',;...,;;::;;'..;'...'::::::::::::lddddd
KKK0Okkko,'.'..'';:cll:;'..,'''.,0kxl::::::::oddddd
KKKKK0kkx:,....'',;,,,,,'. '','..ld0K0xddolc:oddxdd
KKKKKK0Oko,'.. ...',;;,..  .,''..dXNNNXXXXKK0Okxkxx
KKKKKKKK0xc;,..    .. .    .,':xoNNWWNNNXKKKK000OOO
KKKKKKKK0O0Kk..            .ONWxXWWWNNNXXKKKKKKKKKK
KKKKK00KKKKXox'.          ,0WWdXWWWNNNNXXXXXXXXXNXN
kkkO00KXKKXNd0K0ol,,,:c;oONMWdKXNNNNNNX0ko:cldNNNNW
O0KKKKXXXKXXdNXXNWNWWNWWWWWXdKXNNNNNOo;;;;;,;,,0WWW
KKKKKXXXXXXXoNXXNNNWWWWNNXKoXXNNNNWk';',:cc:,,,.cNW
KKKKKXXXXXXNoXXNNNNNXXXXNKdXNWWWWWx;:;,:l::::;,,''X
KKKKXXXXXNNNdKXXXXXXXNNNKdNWWWWWWX:::,:clc:::c:,,,;
</pre>
</center>


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

<code>make && make install</code> should be all that's needed to run on your system.




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
