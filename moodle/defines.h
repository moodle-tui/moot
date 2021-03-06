/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * Part of moodle library. See moodle.h
 *
 * For the users of library convenience, it was decided to add customisible
 * fields to every exposed struct of moodle library. The name of the fields needs
 * to be defined at compile time (e. g. -DMD_CUSTOM_FIELD_ARRAY=data) and it
 * will have type of void *. No fields are added by default.
*/

#ifndef __DEFINES_H
#define __DEFINES_H

#ifdef MD_CUSTOM_FIELD
#define MD_EXTRA_FIELD void *MD_CUSTOM_FIELD;
#else
#define MD_EXTRA_FIELD
#endif

#ifdef MD_CUSTOM_FIELD_ARRAY
#define MD_EXTRA_FIELD_ARRAY void *MD_CUSTOM_FIELD_ARRAY;
#else
#define MD_EXTRA_FIELD_ARRAY
#endif

#ifdef MD_CUSTOM_FIELD_CLIENT
#define MD_EXTRA_FIELD_CLIENT void *MD_CUSTOM_FIELD_CLIENT;
#else
#define MD_EXTRA_FIELD_CLIENT
#endif

#ifdef MD_CUSTOM_FIELD_RICH_TEXT
#define MD_EXTRA_FIELD_RICH_TEXT void *MD_CUSTOM_FIELD_RICH_TEXT;
#else
#define MD_EXTRA_FIELD_RICH_TEXT
#endif

#ifdef MD_CUSTOM_FIELD_FILE
#define MD_EXTRA_FIELD_FILE void *MD_CUSTOM_FIELD_FILE;
#else
#define MD_EXTRA_FIELD_FILE
#endif

#ifdef MD_CUSTOM_FIELD_FILE_SUBMISSION
#define MD_EXTRA_FIELD_FILE_SUBMISSION void *MD_CUSTOM_FIELD_FILE_SUBMISSION;
#else
#define MD_EXTRA_FIELD_FILE_SUBMISSION
#endif

#ifdef MD_CUSTOM_FIELD_TEXT_SUBMISSION
#define MD_EXTRA_FIELD_TEXT_SUBMISSION void *MD_CUSTOM_FIELD_TEXT_SUBMISSION;
#else
#define MD_EXTRA_FIELD_TEXT_SUBMISSION
#endif

#ifdef MD_CUSTOM_FIELD_MOD_ASSIGNMENT_STATUS
#define MD_EXTRA_FIELD_MOD_ASSIGNMENT_STATUS void *MD_CUSTOM_FIELD_MOD_ASSIGNMENT_STATUS;
#else
#define MD_EXTRA_FIELD_MOD_ASSIGNMENT_STATUS
#endif

#ifdef MD_CUSTOM_FIELD_MOD_WORKSHOP_STATUS
#define MD_EXTRA_FIELD_MOD_WORKSHOP_STATUS void *MD_CUSTOM_FIELD_MOD_WORKSHOP_STATUS;
#else
#define MD_EXTRA_FIELD_MOD_WORKSHOP_STATUS
#endif

#ifdef MD_CUSTOM_FIELD_MOD_ASSIGNMENT
#define MD_EXTRA_FIELD_MOD_ASSIGNMENT void *MD_CUSTOM_FIELD_MOD_ASSIGNMENT;
#else
#define MD_EXTRA_FIELD_MOD_ASSIGNMENT
#endif

#ifdef MD_CUSTOM_FIELD_MOD_WORKSHOP
#define MD_EXTRA_FIELD_MOD_WORKSHOP void *MD_CUSTOM_FIELD_MOD_WORKSHOP;
#else
#define MD_EXTRA_FIELD_MOD_WORKSHOP
#endif

#ifdef MD_CUSTOM_FIELD_MOD_RESOURCE
#define MD_EXTRA_FIELD_MOD_RESOURCE void *MD_CUSTOM_FIELD_MOD_RESOURCE;
#else
#define MD_EXTRA_FIELD_MOD_RESOURCE
#endif

#ifdef MD_CUSTOM_FIELD_MOD_URL
#define MD_EXTRA_FIELD_MOD_URL void *MD_CUSTOM_FIELD_MOD_URL;
#else
#define MD_EXTRA_FIELD_MOD_URL
#endif

#ifdef MD_CUSTOM_FIELD_MODULE
#define MD_EXTRA_FIELD_MODULE void *MD_CUSTOM_FIELD_MODULE;
#else
#define MD_EXTRA_FIELD_MODULE
#endif

#ifdef MD_CUSTOM_FIELD_TOPIC
#define MD_EXTRA_FIELD_TOPIC void *MD_CUSTOM_FIELD_TOPIC;
#else
#define MD_EXTRA_FIELD_TOPIC
#endif

#ifdef MD_CUSTOM_FIELD_COURSE
#define MD_EXTRA_FIELD_COURSE void *MD_CUSTOM_FIELD_COURSE;
#else
#define MD_EXTRA_FIELD_COURSE
#endif

#ifdef MD_CUSTOM_FIELD_LOADED_STATUS
#define MD_EXTRA_FIELD_LOADED_STATUS void *MD_CUSTOM_FIELD_LOADED_STATUS;
#else
#define MD_EXTRA_FIELD_LOADED_STATUS
#endif

#endif