-- Test tables for whatever database I'm testing against.
DROP TABLE IF EXISTS scholarships;
CREATE TABLE scholarships( 
	id INTEGER PRIMARY KEY,
	date_posted VARCHAR,
	name VARCHAR,
	award_amount INTEGER,
	address VARCHAR,
	city VARCHAR,
	state VARCHAR,
	zip INTEGER,
	zip_sub INTEGER,
	contact_names VARCHAR,
	phone VARCHAR,
	fax VARCHAR,
	email VARCHAR,
	web_address VARCHAR,
	requirements VARCHAR,
	can_graduate_students_apply INTEGER
);


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
	'Charles & Lucille King Family Foundation Scholarship',
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

