------------------------------------------------------------
-- washburn-mysql.sql
--
-- Create a database and src table in MySQL
------------------------------------------------------------

-- Drop the database
DROP DATABASE IF EXISTS washdb;


-- Create the scholarship db
CREATE DATABASE IF NOT EXISTS washdb;


-- Create a new user
CREATE USER IF NOT EXISTS 'bomb'@'localhost'
	IDENTIFIED BY 'giantbomb';


-- Grant only what's needed
GRANT SELECT, INSERT, DELETE, UPDATE, ALTER, TRIGGER 
	ON washdb.*
	TO 'bomb'@'localhost';


-- Select this?
USE washdb;

	
/* Test tables for whatever database I'm testing against. */
DROP TABLE IF EXISTS washburn;
CREATE TABLE washburn ( 
	id INTEGER NOT NULL,
	date_posted VARCHAR(64),
	name VARCHAR(256),
	award_amount INTEGER,
	address VARCHAR(256),
	city VARCHAR(64),
	state VARCHAR(3),
	zip INTEGER,
	zip_sub INTEGER,
	contact_names VARCHAR(64),
	phone VARCHAR(64),
	fax VARCHAR(64),
	email VARCHAR(64),
	web_address VARCHAR(256),
	requirements VARCHAR(1024),
	can_graduate_students_apply INTEGER,
	PRIMARY KEY (id)
);


INSERT INTO washburn VALUES ( 
	0,
	'2019-08-09',
	'AIChE Minority Affairs Committee Minority Scholarship Awards for College Students',
	1000,
	'120 Wall Street, Flr 23',
	'New York', 
	'NY',
	10005,
	4020,
	'',
	'212-591-7338 ',
	'',
	'awards@aiche.org',
	'http://www.aiche.org/community/awards/minority-affairs-committees-minority-scholarship-awards-college-students',
	'Undergraduates in chemical engineering during the 2017-2018 academic year -Minority group that is under-represented -Minimum GPA of 3.0/4.0 -Applicants financial need as outlined in his/her EFC (Expected Family Contribution). -A resume -Unofficial copy of student transcript -A letter of recommendation from the AIChE student chapter advisor, department chair, or chemical engineering faculty member. -Must submit essay outlining career objectives (300 words, see website for essay requirements) (See Website)',
	0	
);


INSERT INTO washburn VALUES ( 
	1,
	'2023-04-01',
	'Armenian Relief Society Undergraduate Scholarship',
	-1,
	'80 Bigelow Avenue Suite 200',
	'Watertown', 
	'MA',
	2472,
	0,
	'',
	'617-926-3801',
	'',
	'arseastus@gmail.com',
	'http://www.arseastusa.org/Lazarian%20Graduate%20Scholarship.pdf',
	'1. Applicant must be of Armenian descent  2. Applicant must be an undergraduate student completed at least one college semester at an accredited four-year college or university in the United States OR must be enrolled in a two year college and are transferring to a four year college or university as a full time student in the Fall. 3.The following information must be sent with the completed application:  a- Financial aid forms filed with a testing service or schools financial aid office or a copy of the most recent Income Tax Return of applicants parents.  b- Recent official transcript of college grades with raised seal. No faxes or copies will be accepted.  c- Two letters of recommendation. The letters must include a letter from a college professor or advisor, and an affiliate(s) of Armenian organization(s) of which you are a member.  d- Tuition Costs- Include a copy of your schools costs for the academic year. It can be the most recent official copy of the schools bulletin or a statement cost.',
	0	
);


INSERT INTO washburn VALUES ( 
	2,
	'2023-04-01',
	'Charles & Lucille King Family Foundation Scholarship',
	3500,
	'1212 Avenue of the Americas 7th Floor', 
	'New York', 
	'NY',
	10036,
	0,
	'',
	'212-682-2913',
	'',
	'KingScholarships@aol.com',
	'http://www.kingfoundation.org',
	'Must be a junior and senior majoring in television and film at accredited  4-year, degree-granting U.S. colleges and universities - Two page personal statement - 3 letters of recommendation  - Official college transcript',
	1	
);
