-- Cleanup assuming that this is the not the first time we've run this
DROP USER schtest;

DROP TABLESPACE scholarships
	INCLUDING CONTENTS AND DATAFILES;


-- Set error out
WHENEVER sqlerror exit



-- Create the tablespace (the space for the db and its tables)
PROMPT 
PROMPT Creating new tablespace for scholarships db... 

CREATE BIGFILE TABLESPACE scholarships
	DATAFILE 'scholarships.dbf'
	SIZE 20M AUTOEXTEND ON;

PROMPT DONE!



-- Run me as SYSTEM
PROMPT 
PROMPT Creating new user schtest...

--CREATE USER scholarship_test 
CREATE USER schtest 
	IDENTIFIED BY schpass1;

--ALTER USER scholarship_test 
ALTER USER schtest 
	DEFAULT TABLESPACE scholarships
	QUOTA UNLIMITED ON scholarships;

--ALTER USER scholarship_test 
ALTER USER schtest 
	TEMPORARY TABLESPACE temp; 

GRANT 
	CREATE SESSION, 
	CREATE VIEW, 
	ALTER SESSION, 
	CREATE SEQUENCE, 
	CREATE SYNONYM, 
	CREATE DATABASE LINK, 
	RESOURCE,
	UNLIMITED TABLESPACE 
TO schtest;

--GRANT execute ON sys.dbms_stats TO scholarship_test;
--GRANT execute ON sys.dbms_stats TO schtest;

PROMPT 
PROMPT DONE!


-- Connect as scholarship_test and start creating the rest of the stuff
Prompt ******  Connecting as schtest....

CONNECT schtest/schpass1@XE;

ALTER SESSION SET NLS_LANGUAGE=American;

ALTER SESSION SET NLS_TERRITORY=America;

PROMPT DONE!


-- Create test tables 
Prompt ******  Creating SCHOLARSHIPS table ....

CREATE TABLE scholarships (
	scholarship_id INTEGER,
	date_posted VARCHAR(12),
	name VARCHAR(128),
	award_amount INTEGER,
	address VARCHAR(128),
	city VARCHAR(64),
	state VARCHAR(2),
	zip INTEGER,
	zip_sub INTEGER,
	contact_names VARCHAR(64),
	phone VARCHAR(24),
	fax VARCHAR(24),
	email VARCHAR(24),
	web_address VARCHAR(512),
	requirements VARCHAR(1024),
	can_graduate_students_apply INTEGER,
	CONSTRAINT id_pk PRIMARY KEY (scholarship_id)
);

COMMIT;

PROMPT DONE!


-- Populate
Prompt ******  Populating scholarship data....

INSERT INTO scholarships VALUES ( 
	0,
	'2019-08-09',
	'AIChE Minority Affairs Committee Minority Scholarship Awards for College Students',
	'1000',
	'120 Wall Street, Flr 23',
	'New York', 
	'NY',
	'10005',
	'4020',
	'',
	'212-591-7338 ',
	'',
	'awards@aiche.org',
	'http://www.aiche.org/community/awards/minority-affairs-committees-minority-scholarship-awards-college-students',
	'Undergraduates in chemical engineering during the 2017-2018 academic year -Minority group that is under-represented -Minimum GPA of 3.0/4.0 -Applicants financial need as outlined in his/her EFC (Expected Family Contribution). -A resume -Unofficial copy of student transcript -A letter of recommendation from the AIChE student chapter advisor, department chair, or chemical engineering faculty member. -Must submit essay outlining career objectives (300 words, see website for essay requirements) (See Website)',
	0	
);


INSERT INTO scholarships VALUES ( 
	1,
	'2023-04-01',
	'Armenian Relief Society Undergraduate Scholarship',
	-1,
	'80 Bigelow Avenue Suite 200',
	'Watertown', 
	'MA',
	02472,
	0,
	'',
	'617-926-3801',
	'',
	'arseastus@gmail.com',
	'http://www.arseastusa.org/Lazarian%20Graduate%20Scholarship.pdf',
	'1. Applicant must be of Armenian descent  2. Applicant must be an undergraduate student completed at least one college semester at an accredited four-year college or university in the United States OR must be enrolled in a two year college and are transferring to a four year college or university as a full time student in the Fall. 3.The following information must be sent with the completed application:  a- Financial aid forms filed with a testing service or schools financial aid office or a copy of the most recent Income Tax Return of applicants parents.  b- Recent official transcript of college grades with raised seal. No faxes or copies will be accepted.  c- Two letters of recommendation. The letters must include a letter from a college professor or advisor, and an affiliate(s) of Armenian organization(s) of which you are a member.  d- Tuition Costs- Include a copy of your schools costs for the academic year. It can be the most recent official copy of the schools bulletin or a statement cost.',
	0	
);


INSERT INTO scholarships VALUES ( 
	2,
	'2023-04-01',
	'Charles and Lucille King Family Foundation Scholarship',
	'3500',
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

PROMPT DONE!

COMMIT;
