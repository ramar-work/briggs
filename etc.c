const char loadSql[] =
 "DROP TABLE IF EXISTS posts;"
 "DROP TABLE IF EXISTS content;"
 "DROP TABLE IF EXISTS media;"
 "DROP TABLE IF EXISTS media_rel;"
 "DROP TABLE IF EXISTS authors;"

 "CREATE TABLE IF NOT EXISTS posts ("
 "	posts_id INTEGER PRIMARY KEY AUTOINCREMENT,"
 "	title TEXT,"
 "	author_rel INTEGER,"
 "	date_modified DATE,"
 "	date_added DATE"
 ");"

 "CREATE TABLE IF NOT EXISTS content ("
 "	content_id INTEGER PRIMARY KEY AUTOINCREMENT,"
 "	rel_id INTEGER, /*Assigned to posts*/ "
 "	sort_order INTEGER, /*order*/"
 "	content TEXT,"
 "	date_modified DATE,"
 "	date_added DATE"
 ");"

 "CREATE TABLE IF NOT EXISTS media ("
 "	media_id INTEGER PRIMARY KEY AUTOINCREMENT,"
 "	path TEXT,"
 "	date_modified DATE,"
 "	date_added DATE"
 ");"
 
 "CREATE TABLE IF NOT EXISTS media_rel ("
 "	media_rel_id INTEGER PRIMARY KEY AUTOINCREMENT,"
 "	rel_id INTEGER, "
 "	media_id INTEGER"
 ");"

 "CREATE TABLE IF NOT EXISTS authors ("
 "	authors_id INTEGER PRIMARY KEY AUTOINCREMENT,"
 "	name TEXT,"
 "	username TEXT,"
 "	pwd TEXT,"
 "	date_modified DATE,"
 "	date_added DATE"
 ");"
;

const char *authors[] = 
{
 "Antonio R. Collins II",
 "Janice Wilkerson",
 "Trevor Wallace",
 "010100101",
 "Nikhil A. Shah",
 "DJ Trace"
};


const char *authorIDs[] = 
{
 "acolli_2",
 "jWilkerson",
 "trev0rwallz",
 "bottybot",
 "nikhil_a_shah",
 "dj_trace"
};

const char *pwds [] = 
{
	"5d41402abc4b2a76b9719d911017c592", /*hello         */
	"6f2e64d2ee5211666faee8245f1c8e13", /*jinny         */
	"4522b0a844f7ba8a8878b99b0d317584", /*pigeon12      */
	"cc8bf34343e2790f27845f2ba9407a54", /*stix1286      */
	"f97a90439591e925155c801345ee6c3e", /*abacusMan!    */
	"cf449761b9d618e79f07853638290591", /*jamesTKirk!121*/
};


const char *titles[] = 
{
	"zucchini",
	"zurich",
	"yip",
	"yipping",
	"yitbos",
	"abusive",
	"africans",
	"afro",
	"dabbling",
	"dacca",
	"dachshund",
	"garter",
	"garters",
	"garth",
	"dactyl",
	"ymca",
	"babyface",
	"babygirl",
	"babyhood",
	"folklore",
	"folks",
	"folksong",
	"iliad",
	"indefinable",
	"indefinite",
	"indefinitely",
	"ilikeit",
	"ill",
	"babying",
	"gloucester",
	"hannibal",
	"hanoi",
	"hanover",
	"glove",
	"gloved",
	"glover",
	"babyish",
	"yogurt",
	"yoke",
	"yokel",
	"facetious",
	"facets",
	"facial",
	"wrinkle",
	"wrinkled",
};


const char *content[] = 
{
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. In fermentum velit in elit efficitur dignissim. Duis interdum sagittis ex, ac ultrices dolor. Curabitur quis lorem accumsan, egestas ante eu, pharetra velit. Nunc eu lorem ligula. Vivamus eleifend ultrices tellus, ac finibus diam convallis non. Suspendisse in dignissim enim, eget euismod nulla. Pellentesque dolor velit, malesuada vitae sollicitudin non, imperdiet eget sem. Mauris viverra malesuada nulla nec euismod. Aenean malesuada erat nulla, sit amet pretium velit lobortis sed. Donec fermentum dictum vulputate.",
"Integer quis pharetra tortor. Sed ut tellus eu mi consequat aliquam in quis lacus. Aliquam nec dolor quis neque facilisis ultrices vel sit amet tortor. Ut vel elementum risus. Integer sit amet hendrerit urna. Aliquam in pretium massa. Aenean ut dui varius, dignissim ex interdum, efficitur sem. Vivamus quis fermentum sem.",
"Sed quis bibendum dui. Pellentesque rutrum porta diam, accumsan vulputate ante vestibulum quis. Phasellus convallis quam lacinia eros lobortis vehicula. Sed lectus mi, pretium nec malesuada feugiat, sodales quis massa. Donec gravida finibus egestas. Donec suscipit, neque ac ultricies elementum, neque felis fermentum orci, in blandit lacus orci sit amet lacus. Pellentesque auctor ex ac nisi laoreet dapibus. Sed sagittis feugiat sem nec dignissim. Aenean non ante quam. Pellentesque nec blandit tellus.",
"In lectus orci, interdum bibendum neque quis, vulputate finibus orci. Maecenas in dolor semper orci consequat mollis. Nullam quis est eu felis finibus ullamcorper. Fusce sem ante, suscipit eget ex pharetra, suscipit mollis lectus. Nullam vel ligula eu turpis malesuada molestie eu in dolor. In hac habitasse platea dictumst. Aliquam erat volutpat. Sed porttitor vel risus ac facilisis. Ut bibendum tincidunt libero, sit amet consectetur magna ullamcorper sed. Aenean fringilla fringilla finibus. Praesent est nibh, tincidunt nec augue ut, consectetur imperdiet libero. In eget pulvinar quam. Duis imperdiet, enim id aliquet tempus, sapien nisl placerat eros, porta imperdiet arcu mi eget nulla.",
"Cras sit amet massa posuere risus volutpat pellentesque. Nam id tempus nunc. Nulla vestibulum turpis nibh, in pretium ex commodo et. Nunc non volutpat nulla, vel iaculis mauris. Integer luctus, libero at commodo dapibus, odio tortor consectetur ex, vel mattis nisl diam nec magna. Fusce luctus dui sit amet lectus lobortis, quis tempus purus eleifend. Phasellus varius id ipsum eleifend iaculis. Fusce nisl justo, finibus congue congue ac, blandit in eros. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris vel diam nec metus viverra consequat id non ex. Aliquam erat volutpat. Vivamus pharetra et risus eu faucibus.",
"Etiam iaculis, erat et posuere placerat, massa velit ornare enim, vel placerat orci est quis elit. Integer mattis varius sapien sed hendrerit. Nullam condimentum, velit et placerat blandit, purus nibh feugiat nisi, iaculis blandit metus nunc quis leo. Mauris sed nisl dapibus, tristique arcu ut, facilisis nisl. Pellentesque scelerisque at magna sit amet cursus. Phasellus tincidunt vulputate venenatis. Ut a gravida eros. Ut eget maximus arcu, eget dictum sem. Sed dapibus sit amet lacus efficitur congue. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Maecenas lobortis velit augue, sed rhoncus nunc sollicitudin ut. Praesent vel malesuada sem, ac viverra felis. Integer vitae erat sit amet odio rutrum accumsan ut et tortor. Pellentesque id tempus lorem. Vivamus auctor rhoncus orci, vel eleifend neque.",
"Donec consectetur gravida metus, nec porttitor tortor lacinia ac. Etiam ullamcorper vel est ut porta. Etiam condimentum felis ut dictum eleifend. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Fusce fringilla tortor sed sapien hendrerit, vel faucibus odio imperdiet. Sed ac mauris libero. Sed faucibus sollicitudin purus, in tincidunt enim pretium ac. Curabitur lobortis vitae felis vel accumsan. Aenean sit amet eros at mi placerat congue.",
"Fusce molestie, sapien at ullamcorper iaculis, libero urna faucibus nibh, aliquam iaculis mauris nisi in eros. Fusce lacus massa, luctus ac mollis vitae, elementum sit amet justo. Proin et nulla vitae neque consequat lobortis ac nec est. Duis ligula ex, porta ut urna ut, consequat consequat diam. Proin vel velit vel libero egestas commodo id at dolor. Nam laoreet aliquet vulputate. Ut feugiat nisi magna, eget auctor odio imperdiet vitae. Praesent viverra justo justo, lobortis dapibus arcu eleifend a. In felis nisl, gravida sed arcu a, tristique malesuada lacus.",
"Nunc elementum tortor dignissim elit molestie, sit amet ornare tortor pharetra. Proin non efficitur urna. Ut eu quam est. Morbi nec facilisis urna. Cras euismod est quis luctus malesuada. Pellentesque tristique varius enim nec aliquet. In pulvinar volutpat massa blandit porttitor. Vestibulum tincidunt et enim quis consequat. Donec at accumsan urna. Mauris neque tellus, congue eget velit in, lacinia tristique dolor. Sed pharetra lacus in sodales facilisis. Maecenas ullamcorper odio vel diam fringilla, sit amet varius justo gravida. Integer nec felis elementum, elementum elit maximus, vehicula est. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Vestibulum luctus eget dolor vitae rutrum.",
"Integer non blandit neque. Pellentesque ex mi, posuere sed nisl nec, aliquam aliquam orci. Duis faucibus tincidunt felis sit amet molestie. Aenean ac tellus ultricies, fringilla erat nec, dignissim eros. Nulla commodo est est, ullamcorper volutpat mauris malesuada non. Fusce ac odio eros. Maecenas finibus fringilla volutpat. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos.",
"Sed facilisis turpis vitae eros mattis, lobortis bibendum nisl congue. Pellentesque nisl nulla, feugiat ac arcu eget, pharetra interdum augue. Aenean a nisl ut felis dapibus consequat ut a elit. Aliquam vitae gravida tellus, at accumsan ante. Aenean sit amet ex eu erat facilisis rutrum. Donec ac nibh ac neque sodales porta a eu quam. Maecenas sed tincidunt enim.",
"Duis id pharetra lorem. Aliquam pulvinar vulputate pharetra. Sed gravida ut lorem vel bibendum. Aenean vel elit a mi tincidunt blandit ut at ante. Nam aliquet vitae diam vitae ullamcorper. Fusce scelerisque pellentesque lacus, non condimentum nulla sodales vitae. Cras faucibus lorem dictum urna fringilla venenatis. Sed ullamcorper libero sed lacus blandit eleifend. Nunc eros dui, accumsan ut risus eu, dictum aliquet enim. Nulla at commodo est, eget condimentum est.",
"Nam ullamcorper vulputate magna, at auctor ligula interdum id. Mauris eu elit sed elit rutrum scelerisque ut id massa. Fusce sagittis enim in ipsum auctor facilisis. Integer ut sem hendrerit, rhoncus ipsum sed, molestie orci. Sed ac suscipit neque. In gravida vel eros a rhoncus. Vivamus pharetra justo in purus semper ullamcorper. Ut ullamcorper elit tortor, sit amet fermentum urna tristique non. Sed venenatis dolor a dui congue, id congue augue euismod. Integer bibendum lectus eget sem pharetra pellentesque. Aenean semper mi lacus, in vehicula ligula iaculis eu. Donec scelerisque augue vel mauris pretium rutrum. Curabitur non consectetur velit, ut auctor urna. Donec ultrices porta mi et porta.",
"Proin varius varius justo efficitur sollicitudin. Proin id interdum eros. Curabitur a lectus risus. Nullam laoreet placerat purus sed dapibus. Ut tempor semper sapien, quis viverra purus viverra sit amet. Mauris euismod purus at diam pretium faucibus et ac purus. Morbi sed volutpat felis. Curabitur egestas justo eu lectus placerat molestie. Integer cursus sagittis tortor, a suscipit nisi interdum et. Fusce leo elit, vestibulum sit amet cursus a, vulputate non tortor.",
"Nullam accumsan ultrices cursus. Suspendisse tortor erat, egestas id posuere vel, dictum id ex. Nam a elit sed justo accumsan varius. Maecenas euismod felis sit amet quam euismod vulputate. Nullam vulputate elit sapien, a tempor mauris suscipit a. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam vitae ornare mi. Praesent dignissim odio quis orci eleifend suscipit. Pellentesque quis orci nulla. Ut nec convallis elit, nec accumsan ipsum. Aenean pharetra risus ac purus pulvinar tincidunt. Etiam gravida euismod urna, vel aliquet leo faucibus vitae. Nulla facilisi.",
"Nunc commodo pulvinar purus, vel cursus ex molestie eget. Integer non nunc mattis, tristique magna ac, pretium elit. Nam in velit et augue lacinia varius id ac sapien. Etiam purus urna, viverra quis mattis eu, faucibus vitae lacus. Etiam ultrices, eros id dapibus sodales, dui lorem euismod urna, a porttitor eros est a neque. Sed vitae tempus turpis. Praesent bibendum, diam a finibus volutpat, nulla ipsum consequat nisl, vel posuere turpis erat in nisi. Donec id est ac mi tincidunt eleifend. Mauris dictum felis est, sed tempus nunc ornare sit amet. Ut rutrum diam eget dignissim laoreet.",
"Aenean venenatis purus nec massa pharetra, non iaculis risus ullamcorper. Curabitur nisi tellus, tempor a ante et, porta bibendum ex. Praesent tempor ipsum vitae odio hendrerit, a mattis dolor molestie. Suspendisse potenti. Pellentesque pulvinar sollicitudin augue cursus congue. In sodales ultricies libero, sit amet feugiat justo. Pellentesque tempus ut tortor eu placerat. Praesent ipsum odio, viverra et pulvinar sit amet, consectetur eu lacus. Aliquam a sollicitudin nunc, ac tristique libero.",
"Pellentesque et leo metus. Nunc eget leo ac massa lacinia auctor. Mauris sed tincidunt nulla. In malesuada fermentum sapien eget vestibulum. Quisque sit amet varius libero, quis condimentum dui. Aliquam pretium semper ligula eu vulputate. Aliquam erat volutpat. Fusce ut venenatis ex. Nulla facilisi. Ut aliquet quam nec massa placerat rhoncus. Etiam at iaculis felis. Integer ullamcorper erat ipsum, non malesuada leo condimentum sed. Vestibulum placerat nisi ut erat convallis sodales. Duis facilisis ultrices mollis.",
"Morbi et velit augue. Ut venenatis, lorem eu mattis vehicula, elit enim venenatis purus, at aliquet nisl sem ac tellus. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Donec vel diam velit. Proin id nisl faucibus, maximus lacus in, vulputate lacus. Nam sagittis porta diam, eu rhoncus mi viverra vitae. Aliquam cursus, velit sit amet imperdiet ultricies, arcu mauris dapibus eros, quis porta augue libero nec mi. Phasellus maximus dapibus tortor egestas aliquet. Nam sit amet justo eu felis rhoncus feugiat.",
"Sed condimentum odio faucibus, condimentum odio et, vulputate felis. In facilisis fringilla finibus. Aenean tincidunt sapien at enim blandit hendrerit. Duis tristique bibendum libero at maximus. Sed vitae tellus ut sem consectetur consequat sed quis leo. Sed ut laoreet libero, in semper sem. Cras felis elit, feugiat nec semper quis, consectetur nec risus. Vivamus neque lorem, dapibus a eros eget, molestie tempus sem.",
"Etiam ut justo id orci mollis finibus. Nulla neque mauris, fermentum vel odio ac, dapibus facilisis leo. Pellentesque luctus eget magna quis aliquam. Nam eu viverra lacus, et molestie lectus. Etiam efficitur maximus est, vel tempus ex venenatis egestas. Praesent sagittis rhoncus arcu, nec laoreet quam iaculis vitae. Nam luctus consequat congue.",
"Vivamus facilisis sodales lorem nec tristique. Proin condimentum risus metus, vel tincidunt justo aliquam ac. Etiam efficitur purus ac nulla dictum, at tristique tortor sollicitudin. Integer pretium id elit facilisis eleifend. Quisque ultricies mauris quis risus mollis molestie. Interdum et malesuada fames ac ante ipsum primis in faucibus. Donec bibendum turpis felis, sit amet rutrum diam mattis vel. Sed ultricies odio vehicula odio lobortis, sed semper est ultrices. Aenean nibh purus, vehicula vel eros et, tincidunt faucibus nisl. Nulla facilisi. Nullam dolor dui, molestie ut placerat at, semper at leo. Ut tempus vestibulum egestas. Praesent pellentesque fringilla porttitor. Sed feugiat, ipsum sit amet scelerisque laoreet, nibh tellus imperdiet neque, sed ullamcorper ante nulla sed mi. Nullam elementum, sem id sollicitudin eleifend, lorem tellus bibendum metus, non porttitor urna lacus at elit. Aliquam vel mi nisl.",
"Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Mauris sit amet cursus nunc. Sed est urna, faucibus eget eleifend eu, vestibulum quis justo. Nulla eu porta ipsum. Integer eu aliquet nibh, vel semper dui. Aenean ultrices, libero eu aliquam porta, ipsum eros porttitor lorem, vitae porta nulla est at ipsum. Sed viverra fringilla ipsum, non porttitor elit tempus laoreet. Suspendisse eu cursus augue, lobortis dapibus leo. Vivamus lacinia elementum rhoncus. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Mauris non blandit ex, vitae varius sapien. Sed luctus laoreet facilisis. Donec rhoncus sapien ac massa tristique, nec porta mi dapibus.",
"Donec eget tellus ornare, condimentum erat vel, pulvinar sem. Duis tempor, nulla et ultricies posuere, metus risus vestibulum odio, et ullamcorper tellus felis vitae urna. Nullam finibus diam lorem, ut sagittis neque iaculis sit amet. Aenean quis ex sodales, molestie nunc eget, pretium libero. Praesent blandit ex dui, nec consectetur augue cursus in. Nullam libero mi, facilisis quis arcu convallis, accumsan cursus nisl. Phasellus egestas ex et sapien efficitur posuere a et nisi. Quisque ac rutrum magna, a consequat eros. Praesent tincidunt hendrerit congue. Nulla at urna maximus, venenatis massa volutpat, posuere metus. Donec dui dui, euismod et dolor euismod, porta facilisis dui. Curabitur iaculis, ante vel gravida dignissim, nisl tortor mollis nisl, porttitor fermentum ipsum erat vitae enim. Praesent condimentum at velit non gravida.",
"In ultrices eleifend lectus et posuere. Mauris in tellus facilisis, interdum lorem in, vehicula risus. Nunc vitae metus dui. Vivamus euismod pharetra elit et lobortis. Vivamus eu placerat augue. Nam quis molestie nunc. Morbi consectetur eget ex et viverra. Suspendisse ac tincidunt metus, ut dictum tellus. Aliquam tincidunt vestibulum orci, sit amet laoreet eros rhoncus a. Sed ac urna rhoncus, commodo dui eu, placerat sem."
};
