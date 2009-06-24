/**
 * This program unifies param-short_name, param-draw_name, raport-title with special dictionary
 * 
 * Pawe³ Kolega 2008
 * 
 * Dictionary file:
 *
 * <param name="Sieæ:Sterownik:Temperatura zewnêtrzna" short_name="Tzew" draw_name="Temp. zewn"/>
 * ...
 * <param name="Sieæ:Sterownik:przep³yw" short_name="Tzew">
 *	<alt name="Sieæ:Sterownik:Przep³yw systemu"/>
 * </param>
 * ...
 * <param name="Sieæ:Sterownik:zakodowany stan wej¶æ logicznych" short_name="Tzew"/>
 * ...
 * <draw title="Wydajno¶ci ciep³owni">
 *	<alt title="Wydajno¶ci osiedlowe"/>
 * </draw>
 * ...
 * <report title="RAPORT TESTOWY">
 *	<alt title="test"/>
 * </param>
 **/

#include "libpar.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

using namespace std;

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "szarp_config.h"
#include "liblog.h"

#include "szbase/szbname.h"

#include <stdio.h>

#include <stdio.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/valid.h>
#include <libxml/relaxng.h>

#include <getopt.h>
#include <regex.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#define UNIFIER_NAMESPACE_STRING \
"http://www.praterm.com.pl/SZARP/unifier"

#define DIR_SEPARATOR "/"

/** Class replaces occurences of string */

class CStrReplaces{
	public:
	CStrReplaces(){};
	/**
 	 * Function replaces first one occurence of given pattern
  	 * @param _to_replace - string to replace
         * @param _src_replace - (source) pattern to replace
         * @param _dst_replace - destination pattern  
         * @return pointer to replaced string or NULL when source pattern not found (must be freed)
         */
	
	char *str_one_replace(char *_to_replace, char *_src_replace, char *_dst_replace); /* replaces first occurence in string */
	
	/**
 	 * Function replaces all occurences of given pattern
 	 * @param _to_replace - string to replace
 	 * @param _src_replace - pattern to replace
 	 * @param _dst_replace - destination pattern  
 	 * @return pointer to replaced string or NULL when source pattern not found (must be freed) 
 	 */	
	char *str_all_replace(char *_to_replace, char *_src_replace, char *_dst_replace); /* replaces all occurences in strig */
	~CStrReplaces(){};
};

char *CStrReplaces::str_one_replace(char *_to_replace, char *_src_replace, char *_dst_replace){
	int pos;
	string to_replace(_to_replace);
	string tmp_replace;
	string src_replace(_src_replace); 
	string dst_replace(_dst_replace); 
	
	pos = to_replace.find(src_replace);
	if (pos==-1)
		return NULL;

	tmp_replace = to_replace.substr(pos + src_replace.length(), to_replace.length());
	to_replace.resize(pos);	
	to_replace.append(dst_replace);
	to_replace.append(tmp_replace);
	return strdup(to_replace.c_str());
}

char *CStrReplaces::str_all_replace(char *_to_replace, char *_src_replace, char *_dst_replace){
	int pos;
	int position;
	string to_replace(_to_replace);
	string tmp_replace;
	string src_replace(_src_replace);
	string dst_replace(_dst_replace);

	position = 0;
	while (1){
		pos = to_replace.find(src_replace, position);
		if (pos==-1 && position==0) /* not found */
			return NULL;
		else if (pos==-1){ /* found one or more */
			break;	
		}

		tmp_replace = to_replace.substr(pos + src_replace.length(), to_replace.length());
		to_replace.resize(pos);	
		to_replace.append(dst_replace);
		to_replace.append(tmp_replace);
		position = pos + src_replace.length();
	}
	return strdup(to_replace.c_str());
}

	/**
	 * Class for split subnames form param name
	 */

class CParamsReplaces: public CStrReplaces{
	public:
		CParamsReplaces():CStrReplaces(){}

		/**
 		 * Function extracts p1 from p1:p2:p3
 		 * @param name ofparam p1:p2:p3
 		 * @return pointr to p1 or NULL when param is not in p1:p2:p3 format (must be freed)
 		 */
		char *get_p1(char *param);
		/**
 		 * Function extracts p2 from p1:p2:p3
 		 * @param name ofparam p1:p2:p3
 		 * @return pointr to p2 or NULL when param is not in p1:p2:p3 format (must be freed)
 		 */
		char *get_p2(char *param);
		/**
 		 * Function extracts p3 from p1:p2:p3
 		 * @param name ofparam p1:p2:p3
 		 * @return pointr to p3 or NULL when param is not in p1:p2:p3 format (must be freed)
 		 */
		char *get_p3(char *param); 
		~CParamsReplaces(){};
};

char *CParamsReplaces::get_p1(char *param){
	int pos;
	string str(param);
	pos = str.find(":");
	if (pos == -1){
		return NULL;		
	}
	str.resize(pos);
	return strdup(str.c_str());
}

char *CParamsReplaces::get_p2(char *param){
	int pos,pos2;
	string str(param);
	string str2;
	if (pos == -1){
		return NULL;		
	}
	pos2 = str.find(":", pos+1);
	if (pos2 == -1){
		return NULL;		
	}
	str2 = str.substr(pos+1, pos2-pos-1);
	return strdup(str2.c_str());
}

char *CParamsReplaces::get_p3(char *param){
	int pos;
	string str(param);
	string str2;
	pos = str.find(":");
	if (pos == -1){
		return NULL;		
	}
	pos = str.find(":",pos+1);
	if (pos == -1){
		return NULL;		
	}
	str2 = str.substr(pos+1, str.length());
	return strdup(str2.c_str());
}

/** 
 * Class replaces all occurences in formulas
 * it can also shrink and expand formulas from *:*:p3 etc.
 */
class CParamsFormulaReplaces: public CParamsReplaces{
	public:
		CParamsFormulaReplaces():CParamsReplaces(){};
		/**
		 * Function expands params in formula from *:*:p3 or *:p2:p3 to p1:p2:p3
		 * @param param name of param with formula (defined or drawdefinable)
		 * @param formula formula with param
		 * @return pinter to expanded formula (must be freed)
		 */
		char *expand_formula(char *param, char *formula);
		/**
		 * Function searching param whether is present in formula 
		 * @param param_formula name of param with formula
		 * @param param_find searching name of param in formula
		 * @param formula formula with param
		 * @return pinter to expanded formula (must be freed)
		 */	
		int find_param_in_formula(char *param_formula, char *param_find, char *formula);
	
		/**
		 * Function replaces param(s) in formula
		 * note: this function is auxilary for function 'proces_formula' 
		 * because it doesn't support wildcards (*:*:)
		 * @param old_param param to replace
		 * @param new_param replacing param
		 * @param formula formula with param
		 * @return 0 if not found, different than 0 otherwise
		 */
		char *replace_param_in_formula(char *old_param, char *new_param, char *formula);

		/**
		 * Function shrinks params in formula from p1:p2:p3 to *:*:p3 or *:p2:p3 
		 * (other combinations are also supported, but currently SZARP doesn't support thems
		 *  - are commented) 
		 * @param param name of param with formula (defined or drawdefinable)
		 * @param formula formula with param
		 * @return pinter to shrinked formula (must be freed)
		 */
		char *shrink_formula(char *param, char *formula);

		/**
		 * Function replaces param(s) in formula with wildcards (*:*:) support
		 * @param param_formula name of param with formula
		 * @param old_param param to replace
		 * @param new_param replacing param
		 * @param formula formula with param
		 * @return pinter to formula with replace param(s) (must be freed)
		 */
		char *process_formula(char *param_formula, char *old_param, char *new_param, char *formula);
		~CParamsFormulaReplaces(){};
};

int CParamsFormulaReplaces::find_param_in_formula(char *param_formula, char *param_find, char *formula)
{
	char *expanded_formula;
	int result = 0;
	expanded_formula = expand_formula (param_formula, formula) ;
	string s_expanded_formula (expanded_formula);
	string s_param_find("(");
	s_param_find.append(param_find);
	s_param_find.append(")");

	if ((int)s_expanded_formula.find(s_param_find)==-1)
		result = 0;
	else
		result = 1;

	free (expanded_formula);
	return result;
}

char *CParamsFormulaReplaces::expand_formula(char *param, char *formula){
	char *temp = NULL;
	char *temp2 = NULL;
	char *temp3 = NULL;
	char *tempx = NULL;
	char *p1 = get_p1(param);
	char *p2 = get_p2(param);
	char *p3 = get_p3(param);
	if (p1 == NULL|p2 == NULL|p3 == NULL)
		return NULL;  

	string p1s(p1);
	p1s.append(":");
	string p2s(":");
	p2s.append(p2);
	p2s.append(":");
	string p3s(":");
	p3s.append(p3);
	free(p3);
	free(p2);
	free(p1);
	temp = str_all_replace(formula, (char *)":*:", (char *)p2s.c_str()); /* p1 */
	if (temp == NULL)
		temp2 = str_all_replace(formula, (char *)"*:", (char *)p1s.c_str()); 
	else
		temp2 = str_all_replace(temp, (char *)"*:", (char *)p1s.c_str()); 
	
	if (temp2 == NULL && temp!=NULL){
		temp3 = str_all_replace(temp, (char *)":*", (char *)p3s.c_str()); 
	}

	if (temp2 != NULL && temp!=NULL){
		temp3 = str_all_replace(temp2, (char *)":*", (char *)p3s.c_str()); 
	}

	if (temp2 != NULL && temp==NULL){
		temp3 = str_all_replace(temp2, (char *)":*", (char *)p3s.c_str()); 
	}
	if (temp2 == NULL && temp == NULL)
		temp3 = str_all_replace(formula, (char *)":*", (char *)p3s.c_str()); /* p1 */

	if (temp3!=NULL)	
		tempx = strdup(temp3);
	else if (temp2!=NULL)
		tempx = strdup(temp2);
	else if (temp!=NULL)
		tempx = strdup(temp);
	else tempx = strdup(formula);


	if (temp3)
		free (temp3);
	if (temp2)
		free (temp2);
	if (temp)
		free (temp);
	return tempx;
}

char *CParamsFormulaReplaces::replace_param_in_formula(char *old_param, char *new_param, char *formula){
	char *temp;
	string s_old_param("(");
	s_old_param.append(old_param);
	s_old_param.append(")");

	string s_new_param("(");
	s_new_param.append(new_param);
	s_new_param.append(")");

	temp = str_all_replace (formula, (char *)s_old_param.c_str(), (char *)s_new_param.c_str());
	return temp;
}

char *CParamsFormulaReplaces::process_formula(char *param_formula, char *old_param, char *new_param, char *formula){
	char *expanded_formula;
	char *replaced_formula;
	char *shrinked_formula;
	expanded_formula = expand_formula(param_formula, formula);
	replaced_formula = replace_param_in_formula (old_param, new_param, expanded_formula);
	free (expanded_formula);
	shrinked_formula = shrink_formula(param_formula, replaced_formula);
	free (replaced_formula);
	return shrinked_formula ;
}

char *CParamsFormulaReplaces::shrink_formula(char *param, char *formula){
	char *temp = NULL;
	char *temp2 = NULL;
//	char *temp3 = NULL;
	char *tempx = NULL;
	char *p1 = get_p1(param);
	char *p2 = get_p2(param);
	char *p3 = get_p3(param);
	if (p1 == NULL|p2 == NULL|p3 == NULL)
		return NULL;  
	
	string p1s("(");
	p1s.append(p1);
	p1s.append(":");


//	string p2s(":");
//	p2s.append(p2);
//	p2s.append(":");

//	string p3s(":");
//	p3s.append(p3);
//	p3s.append(")");


	string p2s(p1s);
	p2s.append(p2);
	p2s.append(":");


	free(p3);
	free(p2);
	free(p1);

	/* only (*:name 2:name 3) or ( *:*:name 3) */
	temp = str_all_replace(formula, (char *)p2s.c_str(), (char *)"(*:*:"); /* p1 */
	if (temp == NULL){ //Not maximum shrink.
		temp2 = str_all_replace(formula, (char *)p1s.c_str(), (char *)"(*:"); /* p1 */
		if (temp2 == NULL){ //Not shrinked
			tempx = strdup (formula);	
		}else{
			tempx = strdup(temp2);
			free(temp2);
		}


	}else{
		temp2 = str_all_replace(temp, (char *)p1s.c_str(), (char *)"(*:"); /* p1 */
		if (temp2==NULL){
			tempx=strdup(temp);
			free(temp);
		}else{
			tempx=strdup(temp2); 
			free (temp2);
			free (temp);
		}
	}
	return tempx;


/* THIS IS VERY USEFUL SHRINK CODE, BUT UNFORTUNATELY, SZARP DOESN'T SUPPORT IT eg. param1:*:param3 */
/*
	temp = str_all_replace(formula, (char *)p2s.c_str(), (char *)":*:");
	if (temp == NULL)
		temp2 = str_all_replace(formula, (char *)p1s.c_str(), (char *)"(*:"); 
	else
		temp2 = str_all_replace(temp, (char *)p1s.c_str(), (char *)"(*:"); 
	
	if (temp2 == NULL && temp!=NULL){
		temp3 = str_all_replace(temp, (char *)p3s.c_str(), (char *)":*)"); 
	}

	if (temp2 != NULL && temp!=NULL){
		temp3 = str_all_replace(temp2, (char *)p3s.c_str(), (char *)":*)"); 
	}

	if (temp2 != NULL && temp==NULL){
		temp3 = str_all_replace(temp2, (char *)p3s.c_str(), (char *)":*)"); 
	}
	if (temp2 == NULL && temp == NULL)
		temp3 = str_all_replace(formula, (char *)p3s.c_str(), (char *)":*)");

	if (temp3!=NULL)	
		tempx = strdup(temp3);
	else if (temp2!=NULL)
		tempx = strdup(temp2);
	else if (temp!=NULL)
		tempx = strdup(temp);
	else tempx = strdup(formula);

	if (temp3)
		free (temp3);
	if (temp2)
		free (temp2);
	if (temp)
		free (temp);
	return tempx;
*/
}

 #define BUFSZ 16000
 #define COPY_OK 1
 #define COPY_ERROR 0

/**
 * Class supports some simple and elementary file and directory
 * actions (operations)
 */ 
class FileActions{
	public:
	FileActions(){};
	/**
	 * Function copy one file
	 * @param src path to source file
	 * @param dst path to destination file (with destination file name)
	 * @return 0 if fails greater than 0 otherwise
	 */
	int file_copy(char *src, char *dst);
	/**
	 * Function copy directory with files (without subdirs)
	 * @param src path to source directory
	 * @param dst path to destination directory
	 * @return 0 if fails greater than 0 otherwise
	 */
	int dir_copy(char *src, char *dst);

	/**
	 * Function removes files from directory and dir if empty 
	 * (without subdirs)
	 * @param dir path to directory
	 * @return 0 if fails greater than 0 otherwise
	 */
	int dir_remove(char *dir);

	/**
	 * Function has the same functionality as dir_remove
	 * but it dosen't delete directory
	 * @param dir path to directory
	 * @return 0 if fails greater than 0 otherwise
	 */
	int dir_remove_files(char *dir);

	/**
	 * Function moves directory with files (without subdirs)
	 * like 'mv'
	 * @param src path to source directory
	 * @param dst path to destination directory
	 * @return 0 if fails greater than 0 otherwise
	 */
	int dir_move(char *src, char *dst);

	/**
	 * Function creates szbase directories
	 * @param prefix path to szbase directory (e.g. /opt/szarp/byto/szbase)
	 * @param szbase_dirs converted szbase param name from p1:p2:p3 to p1/p2/p3 (by latin2szb)
	 * @return 0 if fails, greater than 0 otherwise
	 */ 
	bool create_szbase_dirs(char *prefix, char *szbase_dirs);

	/**
	 * Function removes szbase directories if empty
	 * @param prefix path to szbase directory (e.g. /opt/szarp/byto/szbase)
	 * @param szbase_dirs converted szbase param name from p1:p2:p3 to p1/p2/p3 (by latin2szb)
	 * @return 0 if fails, greater than 0 otherwise
	 */ 
	bool remove_empty_szbase_dirs(char *prefix, char *szbase_dirs);
	/**
	 * Function checks wheather directory is emty
	 * (has no files and subdirs)
	 * @param dir path to directory
	 * @return 1 if true, 0 otherwise 
	 */ 
	bool is_dir_empty(char *src);
	~FileActions(){};

	private:
		/**
		 * Function retrieves p1 from p1/p2/p3
		 * @param name of param p1:p2:p3
 		 * @return pointr to p1 or NULL when param is not in p1:p2:p3 format (must be freed)
 		 */
		char *get_first_dir(char *szbase_dirs); 
	
		/**
		 * Function retrieves p1/p2 from p1/p2/p3
		 * @param name of param p1:p2:p3
 		 * @return pointr to p1/p2 or NULL when param is not in p1:p2:p3 format (must be freed)
 		 */
		char *get_second_dir(char *szbase_dirs); 
};

char *FileActions::get_first_dir(char *szbase_dirs){
	int pos;
	string str(szbase_dirs);
	pos = str.find(DIR_SEPARATOR);
	if (pos == -1){
		return NULL;		
	}
	str.resize(pos);
	return strdup(str.c_str());
}

char *FileActions::get_second_dir(char *szbase_dirs){
	int pos,pos2;
	string str(szbase_dirs);
	string str2;
	pos = str.find(DIR_SEPARATOR);
	if (pos == -1){
		return NULL;		
	}
	pos2 = str.find(DIR_SEPARATOR, pos+1);
	if (pos2 == -1){
		return NULL;		
	}

	str2 = str.substr(0, pos2 );
	return strdup(str2.c_str());
}

bool FileActions::create_szbase_dirs(char *prefix, char *szbase_dirs){
	string str(prefix);
	str.append(DIR_SEPARATOR);
	str.append(szbase_dirs);
	struct stat Stat;
	char *s;
	if (stat(str.c_str(),&Stat)==0 && !is_dir_empty((char *)str.c_str())){
		printf("Warning. directory '%s' already exists and isn't empty. This operation may corrupt unused data!!!\n",str.c_str());
		if (!dir_remove_files ((char *)str.c_str())){
				printf("Error while removing files from non-empty directory '%s' \n", str.c_str());
				return 0;	
		}

	}
	if (stat(str.c_str(),&Stat)==-1){
		str.clear();
		str.append(prefix);
		str.append(DIR_SEPARATOR);
		s = get_first_dir(szbase_dirs); 
		str.append(s);
		free(s);
		if (stat(str.c_str(),&Stat)==-1){
			/* creating first directory */
			if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
				printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
				return false;
			}
			
			/* creating second directory */
			str.clear();
			str.append(prefix);
			str.append(DIR_SEPARATOR);
			s = get_second_dir(szbase_dirs); 
			str.append(s);
			free(s);	
			if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
				printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
				return false;
			}

			/* creating third directory */
			str.clear();
			str.append(prefix);
			str.append(DIR_SEPARATOR);
			str.append(szbase_dirs);
			if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
				printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
				return false;
			}
		}else{	
			str.clear();
			str.append(prefix);
			str.append(DIR_SEPARATOR);
			s = get_second_dir(szbase_dirs); 
			str.append(s);
			free(s);
			if (stat(str.c_str(),&Stat) == -1){
				/* creating second directory */
				if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
					printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
					return false;
				}
	
				/* creating third directory */
				str.clear();
				str.append(prefix);
				str.append(DIR_SEPARATOR);
					str.append(szbase_dirs);
				if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
					printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
						return false;
				}
			}else{
				/* creating third directory */
				str.clear();
				str.append(prefix);
				str.append(DIR_SEPARATOR);
					str.append(szbase_dirs);
				if (mkdir (str.c_str(),0777) &&  errno && ENOENT){
					printf("Error creating '%s', errno %d (%s)\n", str.c_str(), errno, strerror(errno));
						return false;
				}
	
			     }
			}
	}
	return true;
}

bool FileActions::remove_empty_szbase_dirs(char *prefix, char *szbase_dirs){
	char * s;
	string str(prefix);
	str.append(DIR_SEPARATOR);
	str.append(szbase_dirs);

	if (is_dir_empty ((char *)str.c_str()) == true){
			rmdir (str.c_str());
	}else{
		return false;
	}
	str.clear();
	str.append(prefix);
	str.append(DIR_SEPARATOR);
	s = get_second_dir(szbase_dirs); 
	str.append(s);
	free(s);
	
	if (is_dir_empty ((char *)str.c_str()) == true){
			rmdir (str.c_str());
	}
	else{
		str.clear();
		str.append(prefix);
		str.append(DIR_SEPARATOR);
		s = get_first_dir(szbase_dirs); 
		str.append(s);
		free(s);
	
		if (is_dir_empty ((char *)str.c_str()) == true){
				rmdir (str.c_str());
		}
	}
	return true;
}

bool FileActions::is_dir_empty(char *src){
	struct direct *DirEntryPtr;  /* pointer to a directory entry */
	long dir_ctr = 0;
	DIR *DirPtr = opendir(src);  /* pointer to the directory */
	if (!DirPtr)
		return true;
	while (1) {
      		DirEntryPtr = readdir(DirPtr);
      		if (DirEntryPtr == 0) break;  /* reached end of directory entries */
 		if (strcmp(DirEntryPtr->d_name,".") != 0 &&
          	strcmp(DirEntryPtr->d_name,"..") != 0)  {
			dir_ctr++;
			break;
		}
	}
	closedir(DirPtr);
	if (dir_ctr) return false;
	return true;
}

int FileActions::dir_copy(char *src, char *dst){
	struct direct *DirEntryPtr;  /* pointer to a directory entry */
	DIR *DirPtr = opendir(src);  /* pointer to the directory */
	char tmp_src[1024];
	char tmp_dst[1024];
	if (!DirPtr)
		return 0;

	while (1) {
      		DirEntryPtr = readdir(DirPtr);
      		if (DirEntryPtr == 0) break;  /* reached end of directory entries */
 		if (strcmp(DirEntryPtr->d_name,".") != 0 &&
          	strcmp(DirEntryPtr->d_name,"..") != 0)  {
			sprintf(tmp_src,"%s/%s", src, DirEntryPtr->d_name);
			sprintf(tmp_dst,"%s/%s", dst, DirEntryPtr->d_name);
			if (!file_copy(tmp_src,tmp_dst)){
				printf ("Error. Fail to copy file from '%s' to '%s' \n", tmp_src,tmp_dst);
				return 0;
			}
		}
	}
	closedir(DirPtr);
	return 1;
}

int FileActions::dir_remove(char *dir){
	struct direct *DirEntryPtr;  /* pointer to a directory entry */
	DIR *DirPtr = opendir(dir);  /* pointer to the directory */
	char tmp[1024];
	if (!DirPtr)
		return 0;

	while (1) {
      		DirEntryPtr = readdir(DirPtr);
      		if (DirEntryPtr == 0) break;  /* reached end of directory entries */
 		if (strcmp(DirEntryPtr->d_name,".") != 0 &&
          	strcmp(DirEntryPtr->d_name,"..") != 0)  {
			sprintf(tmp,"%s/%s", dir, DirEntryPtr->d_name);
			unlink (tmp);
		}
	}
	closedir(DirPtr);
	
	if (rmdir(dir) && errno && ENOENT){
		printf("Error removing '%s', errno %d (%s)", dir, errno, strerror(errno));
	}
	return 1;
}

int FileActions::dir_remove_files(char *dir){
	struct direct *DirEntryPtr;  /* pointer to a directory entry */
	DIR *DirPtr = opendir(dir);  /* pointer to the directory */
	char tmp[1024];
	if (!DirPtr)
		return 0;

	while (1) {
      		DirEntryPtr = readdir(DirPtr);
      		if (DirEntryPtr == 0) break;  /* reached end of directory entries */
 		if (strcmp(DirEntryPtr->d_name,".") != 0 &&
          	strcmp(DirEntryPtr->d_name,"..") != 0)  {
			sprintf(tmp,"%s/%s", dir, DirEntryPtr->d_name);
			unlink (tmp);
		}
	}
	closedir(DirPtr);
	return 1;
}

int FileActions::dir_move(char *src, char *dst){
	if (!dir_copy(src, dst))
		return 0;
	if (is_dir_empty(src))
		dir_remove(src);
	return 1;
}

int FileActions::file_copy( char *src, char *dst)
 {
 char            *buf;
 FILE            *fi;
 FILE            *fo;
 unsigned        amount;
 unsigned        written;
 int             result;
 
 struct stat Stat;
 stat(src,&Stat);

 buf = new char[BUFSZ];

 fi = fopen( src, "rb" );
 fo = fopen( dst, "wb" );
 
 result = COPY_OK;
 if  ((fi == NULL) || (fo == NULL) )
   {
   result = COPY_ERROR;
   if (fi != NULL) fclose(fi);
   if (fo != NULL) fclose(fo);
   return 0;
   }
 
 if (result == COPY_OK)
   {
   do
     {
     amount = fread( buf, sizeof(char), BUFSZ, fi );
     if (amount)
       {
       written = fwrite( buf, sizeof(char), amount, fo );
       if (written != amount)
         {
         result = COPY_ERROR; // out of disk space or some other disk err?
         }
       }
     } // when amount read is < BUFSZ, copy is done
   while ((result == COPY_OK) && (amount == BUFSZ));
   fclose(fi);
   fclose(fo);
   }
 delete [] buf;
 chmod(dst,Stat.st_mode);
 chown(dst, Stat.st_uid, Stat.st_gid);//Nie dziala
 return(result);
}

/** Class - list alternative names of reports */
class TAltDictReport{
	public:
		TAltDictReport(char * _alt_name, char * _alt_description, char * _alt_filename) 
		{ alt_name = strdup(_alt_name);
		if (_alt_description)
			alt_description = strdup(_alt_description);
		else
			alt_description = NULL;
		if (_alt_filename)
			alt_filename = strdup(_alt_filename);
		else
			alt_filename = NULL;
			_next = NULL;
		}

		TAltDictReport *_next ;  /** <pointer to next item in list*/
		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TAltDictReport *Append(TAltDictReport * p);
	
		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TAltDictReport *GetNext();
	
		/**
		 * Function searches list by alt name
		 * @param _alt_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TAltDictReport *SearchByAltName(char *_alt_name, bool icase);

		char *alt_name ; /** <alt name */
		char *alt_description ; /** <alt descrption */
		char *alt_filename ; /** <alt filename */

		~TAltDictReport(){
			if (alt_filename)
				free (alt_filename); 
			if (alt_description)
				free (alt_description); 
			free (alt_name); 
		};
};

TAltDictReport *TAltDictReport::Append(TAltDictReport * p)
{
    TAltDictReport *t = this;
    while (t->_next)
        t = t->_next;
    t->_next = p;
    return p;
}

TAltDictReport *TAltDictReport::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TAltDictReport* TAltDictReport::SearchByAltName(char *_alt_name, bool icase)
{
	for (TAltDictReport *t = this; t; t = t->_next){
			if (!strcmp(t->alt_name,_alt_name) ){
				return t;
			}	
	}
	return NULL;
}

/** Class - list keps names of report */
class TDictReport{
	public:
		TDictReport(char * _name, char * _description, char * _filename)
	{
	name = strdup (_name);
	if (_description)
		description = strdup (_description); 
	else
		description = NULL;
	if (_filename)
		filename = strdup (_filename);
	else
		filename = NULL;
	alt_dict_reports = NULL;
	p = NULL;
	_next = NULL;
	}
		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TDictReport *Append(TDictReport * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictReport *SearchByName(char *_name, bool icase) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TDictReport *GetNext();
		TDictReport *_next; /** <pointer to next item in list*/ 
		char *name; /** <name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		char *description; /** <name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		char *filename; /**<path to filename with report file */	
		TAltDictReport *alt_dict_reports; /**<list with alternate params*/
		
		/**
		 * Function gets next pointer to alternate name
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */ 
		TAltDictReport *GetAltReport();
		TAltDictReport *p; /**<auxilary pointer*/
		
		void InsertAltReport(char *_alt_name, char *_alt_description, char *_alt_filename);
		~TDictReport()
		{
			if (filename)
				free (filename);
			if (description)
				free (description);
			free(name); 
			TAltDictReport *tmp_alt_dict_report;
			while (alt_dict_reports){
				tmp_alt_dict_report = alt_dict_reports;
				alt_dict_reports=alt_dict_reports->_next;
				delete tmp_alt_dict_report;
			}
		}
};

TAltDictReport *TDictReport::GetAltReport()
{
	if (alt_dict_reports)
		return (alt_dict_reports->_next);
	return NULL;
}

TDictReport *TDictReport::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TDictReport *
TDictReport::Append(TDictReport * p)
{
    TDictReport *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TDictReport* TDictReport::SearchByName(char *_name, bool icase)
{
	for (TDictReport *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}

void TDictReport::InsertAltReport(char *_alt_name, char *_alt_description, char *_alt_filename){
	if (p == NULL)
		p = alt_dict_reports = new TAltDictReport(_alt_name, _alt_description, _alt_filename);
	    else
		p = p->Append (new TAltDictReport(_alt_name, _alt_description, _alt_filename));
}

/* class keeps alternative patterns of param */
class TAltDictDraw{
	public:
		TAltDictDraw(char * _alt_name) 
		{ alt_name = strdup(_alt_name);_next = NULL;}
		TAltDictDraw *_next ;  /** pointer to next item in list*/

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TAltDictDraw *Append(TAltDictDraw * p);

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TAltDictDraw *GetNext();

		/**
		 * Function searches list by alt name
		 * @param _alt_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TAltDictDraw *SearchByAltName(char *_alt_name, bool icase);

		char *alt_name ; /** <alternate name */
		~TAltDictDraw(){free (alt_name);};
		
};

TAltDictDraw *TAltDictDraw::Append(TAltDictDraw * p)
{
    TAltDictDraw *t = this;
    while (t->_next)
        t = t->_next;
    t->_next = p;
    return p;
}

TAltDictDraw *TAltDictDraw::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TAltDictDraw* TAltDictDraw::SearchByAltName(char *_alt_name, bool icase)
{
	for (TAltDictDraw *t = this; t; t = t->_next){
			if (!strcmp(t->alt_name,_alt_name) ){
				return t;
			}	
	}
	return NULL;
}

/** Class - list kepps names of draws */
class TDictDraw{
	public:
		TDictDraw(char * _name)
	{
	name = strdup (_name); 
	alt_dict_draws = NULL;
	p = NULL;
	_next = NULL;
	}

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TDictDraw *Append(TDictDraw * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictDraw *SearchByName(char *_name, bool icase) ;

		/**
		 * Function searches list by draw_name
		 * @param _draw_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictDraw *SearchByDrawName(char *_draw_name, bool icase) ;

		/**
		 * Function searches list by short_name
		 * @param _short_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictDraw *SearchByShortName(char *_short_name, bool icase) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TDictDraw *GetNext();
		TDictDraw *_next; /** pointer to next item in list*/ 
		char *name; /** name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		TAltDictDraw *alt_dict_draws; /**<list of laternate draws */
		
		/**
		 * Function gets next pointer to alternate name
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */ 
		TAltDictDraw *GetAltDraw();
		TAltDictDraw *p; /** <auxilary pointer */
		
		void InsertAltDraw(char *_alt_name);
		~TDictDraw()
		{
			free(name); 
			TAltDictDraw *tmp_alt_dict_draw;
			while (alt_dict_draws){
				tmp_alt_dict_draw = alt_dict_draws;
				alt_dict_draws=alt_dict_draws->_next;
				delete tmp_alt_dict_draw;
			}
		}
};

TAltDictDraw *TDictDraw::GetAltDraw()
{
	if (alt_dict_draws)
		return (alt_dict_draws->_next);
	return NULL;
}

TDictDraw *TDictDraw::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TDictDraw *
TDictDraw::Append(TDictDraw * p)
{
    TDictDraw *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TDictDraw* TDictDraw::SearchByName(char *_name, bool icase)
{
	for (TDictDraw *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}

void TDictDraw::InsertAltDraw(char *_alt_name){
	if (p == NULL)
		p = alt_dict_draws = new TAltDictDraw(_alt_name);
	    else
		p = p->Append (new TAltDictDraw(_alt_name));
}

/* class keeps alternative patterns of param */
class TAltDictParam{
	public:
		TAltDictParam(char * _alt_name) 
		{ alt_name = strdup(_alt_name);_next = NULL;}
		TAltDictParam *_next ; /** pointer to next item in list*/

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TAltDictParam *Append(TAltDictParam * p);

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TAltDictParam *GetNext();
	
		/**
		 * Function searches list by alt name
		 * @param _alt_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TAltDictParam *SearchByAltName(char *_alt_name, bool icase);

		char *alt_name ;
		~TAltDictParam(){free (alt_name);};

};

TAltDictParam *TAltDictParam::Append(TAltDictParam * p)
{
    TAltDictParam *t = this;
    while (t->_next)
        t = t->_next;
    t->_next = p;
    return p;
}

TAltDictParam *TAltDictParam::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TAltDictParam* TAltDictParam::SearchByAltName(char *_alt_name, bool icase)
{
	for (TAltDictParam *t = this; t; t = t->_next){
			if (!strcmp(t->alt_name,_alt_name) ){
				return t;
			}	
	}
	return NULL;
}

/* class keeps param name, draw_name, short name and list of alternative patterns of param */
class TDictParam{
	public:
		TDictParam(char * _name, char *_draw_name, char *_short_name)
	{
	name = strdup (_name);
	if (_draw_name)
	draw_name = strdup (_draw_name); 
	else
	draw_name = NULL;
	if (_short_name)
	short_name = strdup (_short_name);
	else
	short_name = NULL;
	alt_dict_params = NULL;
	p = NULL;
	_next = NULL;
	}

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TDictParam *Append(TDictParam * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictParam *SearchByName(char *_name, bool icase) ;

		/**
		 * Function searches list by draw_name
		 * @param _draw_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictParam *SearchByDrawName(char *_draw_name, bool icase) ;

		/**
		 * Function searches list by short_name
		 * @param _short_name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TDictParam *SearchByShortName(char *_short_name, bool icase) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TDictParam *GetNext();
		TDictParam *_next; /** pointer to next item in list*/ 
		char *name; /** name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		char *draw_name; /** draw_name (pattern) of param eg. "Temp. zewn" */
		char *short_name; /** short_name (pattern) of param eg. "Tzewn" */
		TAltDictParam *alt_dict_params; /** <list of laternate names of params */
		
		/**
		 * Function gets next pointer to alternate name
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */ 	
		TAltDictParam *GetAltParam();
		TAltDictParam *p; /** <auxilary pointer */
		
		void InsertAltParam(char *_alt_name);
		~TDictParam()
		{
			if (short_name)
				free(short_name); 
			if (draw_name)
				free(draw_name); 
			free(name);
			TAltDictParam *tmp_alt_dict_param;
			while (alt_dict_params){
				tmp_alt_dict_param = alt_dict_params;
				alt_dict_params=alt_dict_params->_next;
				delete tmp_alt_dict_param;
			}
		}
};

TAltDictParam *TDictParam::GetAltParam()
{
	if (alt_dict_params)
		return (alt_dict_params->_next);
	return NULL;
}

TDictParam *TDictParam::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TDictParam *
TDictParam::Append(TDictParam * p)
{
    TDictParam *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TDictParam* TDictParam::SearchByName(char *_name, bool icase)
{
	for (TDictParam *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}

TDictParam* TDictParam::SearchByDrawName(char *_draw_name, bool icase)
{
	for (TDictParam *t = this; t; t = t->_next){
			if (!strcmp(t->draw_name,_draw_name) ){
				return t;
			}	
	}	
	return NULL;
}

TDictParam* TDictParam::SearchByShortName(char *_short_name, bool icase)
{
	for (TDictParam *t = this; t; t = t->_next){
			if (!strcmp(t->short_name,_short_name) ){
				return t;
			}	
	}
	return NULL;
}

void TDictParam::InsertAltParam(char *_alt_name){
	if (p == NULL)
		p = alt_dict_params = new TAltDictParam(_alt_name);
	    else
		p = p->Append (new TAltDictParam(_alt_name));
}

/** Class program configuration */
class TProgramConfig{
	public:
		TProgramConfig(){
			m_Output=NULL; 
			f_m_Output=0; 
			m_Alt=NULL;
			f_m_Alt=0; 
			f_m_Verbose=0;
			f_m_Help=0;
			f_m_Params=0;
			f_m_Shorts=0;
			f_m_Draws=0;
			f_m_Reports=0;
			f_m_All=0;
			f_m_Check=0;
			path = NULL;
			dir = NULL;
		};
	
		/**
		 * Function parses command line (with get opt)
		 * @param argc amount of params in command line (from main)
		 * @param **argv point to list with params from command line (from main)
		 * @return 1 i success, 0 if fails
		 */
		int ParseCommandLine(int argc, char **argv);

		/**
		 * Function returns path to prefix
		 * @return pointr to path
		 */
		char *GetSzarpPrefixPath();
	
		/**
		 * Function returns path to configuration (e.g. /opt/szarp/byto/config/params.xml)
		 * @return pointer to path
		 */
		char *GetPath(){ if (path) return strdup(path); return NULL;};

		/**
		 * Function returns path to database (e.g. /opt/szarp/byto/szbase)
		 * @return pointer to path
		 */
		char *GetDir(){ if (dir) return strdup(path); return NULL;};

		char IsOutput(){return f_m_Output;};	
		
		char *GetOutput(){return strdup(m_Output);};
		
		char *GetAlt(){return strdup(m_Alt);};
		
		char IsAlt(){return f_m_Alt;};	
		
		char IsParams(){return f_m_Params;};
		
		char IsShorts(){return f_m_Shorts;};
		
		char IsDraws(){return f_m_Draws;};
		
		char IsReports(){return f_m_Reports;};
		
		char IsAll(){return f_m_All;};
		
		char IsCheck(){return f_m_Check;};
		
		char IsVerbose(){return f_m_Verbose;};
		
		char IsHelp(){return f_m_Help;};
		
		char IsICase(){return f_m_ICase;};


		/**
		 * Function shows usage message
		 */
		void Usage();
	
		/**
		 * Function shows error message
		 */
		void Message1();
	
		/**
		 * Function shows error message
		 */
		void Message2();
		
		/**
		 * Function shows error message
		 */
		void Message3();

		char *path; /**< path to configuration file*/
		char *dir; /**< path to szbase directory */ 
		char *m_Output; /**< output params.xml file */
		char f_m_Output; /**< output params.xml flag (presents) */
		char *m_Alt; /**< path to unifier.xml file */
		char f_m_Alt; /**< path to unifier.xml flag (presents) */
		char f_m_Params; /**< help flag (presents)*/
		char f_m_Shorts; /**< process param->draw_name and param->short_name (presents) */
		char f_m_Draws;  /**< process draw->title (presents) */ 
		char f_m_Reports; /**< process raport->title and raport->file and raport->description flag (presents) */  
		char f_m_All; /**< process all (presents) flag */
		char f_m_Check; /**< (presents) flag */

		char f_m_Verbose;/**< verbose flag (presents)*/
		char f_m_Help;/**< help flag (presents)*/
		char f_m_ICase;/**< Ignore case flag (presents)*/
		
		~TProgramConfig(){if (dir) free (dir); if (path) free (path);};
};

void TProgramConfig::Usage()
{
	cout << "Unifier - program allows to change (unify) some tags in params.xml to e.g. transaltions"<<endl;
	cout << "          see documentation for details"<<endl;
	cout<<""<<endl;
	cout << "!!!BEFORE USING PROGRAM IS STRONGLY RECOMMENDED TO BACKUP ALL PROCESSING DATABASE AND CONFIGURATION!!!"<<endl;
	cout<<""<<endl;
	cout << "Usage: "<<endl;
	cout << "unifier [-Dprefix=prefix] <-o|--output <processed.xml>> <-a|--alts <alts.xml>> [-p|--params] [-s|--shorts] [-d|--draws] [-r|--reports] [-l|--all] [-c|--check] [-v|--verbose] [-h|--help]"<<endl;
	cout << "-Dprefix=prefix - optional prefix of processing database. Default is active prefix (hostname)"<<endl;
	cout << " -o|--output <processed.xml> - output params.xml file after processing it's strongly recommend that the name and location are differ than source params.xml file"<<endl;
	
	cout << " -a|--alt <alts.xml> - input xml file with translation rules to processing names of params"<<endl;
	cout << "-p|--params - process tags param->name and all occurences in configuration (eg. send, drawdefinable, rpn). Additionally it's modify database. This is very complex operation and should be used with care"<<endl;
	cout << "-s|--shorts - process tags param->draw_name, param->short_name"<<endl;
	cout << "-d|--draws - process tags draw->title"<<endl;
	cout << "-r|--reports - process tags raport->title, raport->description"<<endl;
	cout << "-l|--all - process tags param->name, param->draw_name, param->short_name, draw->title, raport->title, raport->description"<<endl;
	cout << "-c|--check - checks if given configuration has all patterns defined in file with translation rules. This option is recommended before proper processing database to preserve from fails"<<endl;
	cout <<"-i|--ignore-case - Ignore case mode."<<endl;
	cout <<"-v|--verbose - verbose mode"<<endl;
	cout <<"-h|--help - this screen"<<endl;
}

char *TProgramConfig::GetSzarpPrefixPath()
{
	char buf[100];
	if (!dir)
		return NULL;
        char *prefix = libpar_getpar("", "config_prefix", 1);
        memset(buf,0,sizeof(buf));
        strcat(buf,PREFIX);
        strcat(buf, DIR_SEPARATOR);
        strcat(buf,prefix);
        free (prefix);
	return strdup (buf);
}


void TProgramConfig::Message1()
{
cout <<"Error. Option -o|--output <processed.xml> is obligatory"<<endl;
}

void TProgramConfig::Message2()
{
cout <<"Error. Option <-a|--alts <alts.xml>> is obligatory"<<endl;
}

void TProgramConfig::Message3()
{
cout <<"Error. Nothing to processing."<<endl; 
cout <<"Least one of these options [-p|--params] [-s|--shorts] [-d|--draws] [-r|--reports] [-l|--all] must be active"<<endl;
}

int TProgramConfig::ParseCommandLine(int argc, char **argv){
	int option_index = 0;
	int c;
	int i;

	static struct option long_options[] = {
                   {"output", 1, 0, 'o'}, 
                   {"alt", 1, 0, 'a'},
                   {"params", 0, 0, 'p'},
		   {"shorts", 0, 0, 's'},
		   {"draws", 0, 0, 'd'},
		   {"reports", 0, 0, 'r'},
		   {"all", 0, 0, 'l'},
		   {"check", 0, 0, 'c'},
		   {"confirm", 0, 0, 'f'},
		   {"verbose", 0, 0, 'v'},
		   {"help", 0, 0, 'h'},
		   {"ignore-case", 0, 0, 'i'},
		   {0, 0, 0, 0}
         };

        loginit_cmdline(2, NULL, &argc, argv);
        log_info(0);
	libpar_read_cmdline(&argc, argv);
        libpar_init();

	path = libpar_getpar("", "IPK", 0);
	if (path == NULL) {
		sz_log(0, "'IPK' not found in szarp.cfg");
		return 0;
	}
	dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		sz_log(0, "'datadir' not found in szarp.cfg");
		return 0;
	}
	for (i=1;i<argc;i++){	
		if ((argv[i][0]=='-')&&
			(argv[i][1]=='D')&&	
			(argv[i][2]=='p')&&
			(argv[i][3]=='r')&&
			(argv[i][4]=='e')&&
			(argv[i][5]=='f')&&
			(argv[i][6]=='i')&&
			(argv[i][7]=='x')&&
			(argv[i][8]=='=')){
			for (int j=0;j<(int)strlen(argv[i])+4;j++){
				argv[i][j]=0x20;	
			}
		}
	}
	while ((c = getopt_long (argc, argv, "o:a:psdrlcvhi",long_options,&option_index)) != -1){
	 	switch (c){
			case 'o':
				f_m_Output=1;
				m_Output=strdup(optarg);
			break;
			case 'a':
				f_m_Alt=1;
				m_Alt=strdup(optarg);
			break;
			case 'p':
				f_m_Params=1;
			break;
			case 's':
				f_m_Shorts=1;
			break;
			case 'd':
				f_m_Draws=1;
			break;
			case 'r':
				f_m_Reports=1;
			break;
			case 'l':
				f_m_All=1;
			break;
			case 'c':
				f_m_Check=1;
			break;
			case 'v':
				f_m_Verbose=1;
			break;
			case 'h':
				f_m_Help=1;
			break;
			case 'i':
				f_m_ICase=1;
			break;
		}
	}

	if (f_m_All){	
		f_m_Reports=1;
		f_m_Draws=1;
		f_m_Shorts=1;
		f_m_Params=1;
	}

	if (argc < 2){
		Usage ();
		return 0;
	}

	if (f_m_Help){
		Usage ();
		return 0;
	}

	if (!f_m_Output){
		Message1();
		return 0;
	}

	if (!f_m_Alt){
		Message2();
		return 0;
	}

	if (!f_m_Params && !f_m_Shorts && !f_m_Draws && !f_m_Reports && !f_m_All){
		Message3();
		return 0;
	}
	return 1;
}

/**
 * Class replaces few sections e.g. param, draw_name, short_name and save changes toXML file.
 */ 
class TXmlReplace{
	public:
	TXmlReplace(char *_src_xml_file, char *_dst_xml_file, char *_nspace, char *_param){
		 src_xml_file = strdup (_src_xml_file);
		 dst_xml_file = strdup (_dst_xml_file);
		 nspace = strdup (_nspace);
		 param = strdup (_param);
	};
		/**
		 * Function replaces old param with new param
		 * @param new_param name of new param
		 */ 
		void ReplaceParam(char *new_param);

		/**
		 * Function replaces old short_name with new short_name (in tag param)
		 * @param new_short_name new short_name
		 */ 
		void ReplaceShortName(char *new_short_name);

		/**
		 * Function replaces old draw_name with new short_name (in tag param)
		 * @param new_short_name new short_name
		 */ 
		void ReplaceDrawName(char *new_draw_name);

		/**
		 * Function replaces old draw_title with new draw_title (in tag draw)
		 * @param old_draw_title old draw title to replace
		 * @param new_draw_title new draw_title
		 */ 
		void ReplaceDrawTitle(char * old_draw_title, char *new_draw_title);

		/**
		 * Function replaces old draw_title with new draw_title (in tag draw)
		 * @param new_short_name new short_name
		 */ 
		void ReplaceSendParam(char *new_send);

		/**
		 * Function replaces old formula with new formula (in tag define)
		 * @param new_formula new formula
		 */ 
		void ReplaceFormula(char *new_formula);
	
	char *src_xml_file; /** <path to source (params)xml file */
	char *dst_xml_file; /** <path to destination (params)xml file */ 
	char *nspace; /** namespace in (params)xml */
	char *param;
	~TXmlReplace(){
		if (param)
			free (param);
		if (nspace)
			free(nspace);
		if (dst_xml_file)
			free(dst_xml_file);
		if (src_xml_file)
			free(src_xml_file);
	}
	private:
		/**
		 * Main function to replacing
		 * @param xpath x path with source param name
		 * @param dst_name destination name of tag
		 * @param dst_value destination value
		 */ 
		void XmlProcess(char *xpath, char *dst_name, char *dst_value);
};

void TXmlReplace::XmlProcess(char *xpath, char *dst_name, char *dst_value){
		xmlDocPtr doc = xmlParseFile(src_xml_file);	
	
	if (doc == NULL) {
			fprintf(stderr, "Can't load configuration file %s", src_xml_file);
			return ;
		}
		xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
		xmlXPathRegisterNs(ctx, BAD_CAST "ipk", BAD_CAST nspace);
	char *expr;
//	asprintf(&expr, "//ipk:param[@name = 'Kocio³ 1:Sterownik:temperatura zadana']");
	asprintf(&expr, "//ipk:%s", xpath);

	xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST toUTF8(expr), ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "Internal error. I couldn't compile xpath %s", expr);
		return ;
	}

	if (!xpath_obj->nodesetval || (xpath_obj->nodesetval->nodeNr < 1)) { /* nie znalezlismy nic */
		xmlXPathFreeObject(xpath_obj);
		printf("Internal error. I can't process file with following xpath %s\n",expr);
		return;	
	}
	
	if (expr)
		free(expr);

int i;

for (i=0;i<xpath_obj->nodesetval->nodeNr;i++)
{
//	xmlSetProp(xpath_obj->nodesetval->nodeTab[i], BAD_CAST dst_name, BAD_CAST dst_value);
	xmlSetProp(xpath_obj->nodesetval->nodeTab[i], BAD_CAST dst_name, toUTF8(dst_value));
}
	xmlXPathFreeObject(xpath_obj);
	xmlSaveFormatFileEnc(dst_xml_file, doc, "ISO-8859-2", 1);
	xmlFreeDoc(doc);
}

void TXmlReplace::ReplaceFormula(char *new_formula){
	char *xpath;
	asprintf(&xpath,(char *)"param[@name = '%s']/ipk:define", param);
	XmlProcess(xpath, (char *)"formula", new_formula);
	free(xpath);
}

void TXmlReplace::ReplaceParam(char *new_param){
	char *xpath;
	asprintf(&xpath,(char *)"param[@name = '%s']",param);
	XmlProcess(xpath, (char *)"name", new_param);
	free(xpath);
}

void TXmlReplace::ReplaceShortName(char *new_short_name){
	char *xpath;
	asprintf(&xpath,(char *)"param[@name = '%s']",param);
	XmlProcess(xpath, (char *)"short_name", new_short_name);
	free(xpath);
}

void TXmlReplace::ReplaceDrawName(char *new_draw_name){
	char *xpath;
	asprintf(&xpath,(char *)"param[@name = '%s']",param);
	XmlProcess(xpath, (char *)"draw_name", new_draw_name);
	free(xpath);
}

void TXmlReplace::ReplaceDrawTitle(char *old_draw_title, char *new_draw_title){
	char *xpath;
	asprintf(&xpath,(char *)"draw[@title = '%s']",old_draw_title);
	XmlProcess(xpath, (char *)"title", new_draw_title);
	if (xpath)
		free(xpath);
}

void TXmlReplace::ReplaceSendParam(char *new_send){
	char *xpath;
	asprintf(&xpath, (char *)"send[@param = '%s']",param);
	XmlProcess(xpath, (char *)"param", new_send);
	free(xpath);
}

/* Class keeps unique names of param */
class TUniqueParam{
	public:
		TUniqueParam(char * _name )
	{
	name = strdup (_name);
	_next = NULL;
	}

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TUniqueParam *Append(TUniqueParam * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TUniqueParam *SearchByName(char *_name) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TUniqueParam *GetNext();
		TUniqueParam *_next; /** pointer to next item in list*/ 
		char *name; /** name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		
		~TUniqueParam()
		{
			if (name)
				free(name); 
		}
};

TUniqueParam *TUniqueParam::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TUniqueParam *
TUniqueParam::Append(TUniqueParam * p)
{
    TUniqueParam *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TUniqueParam* TUniqueParam::SearchByName(char *_name)
{
	for (TUniqueParam *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}

/** Class - list keeps unique names of draws */
class TUniqueDraw{
	public:
		TUniqueDraw(char * _name )
	{
	name = strdup (_name);
	_next = NULL;
	}

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TUniqueDraw *Append(TUniqueDraw * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TUniqueDraw *SearchByName(char *_name) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TUniqueDraw *GetNext();
		TUniqueDraw *_next; /** <pointer to next item in list*/ 
		char *name; /** <name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		
		~TUniqueDraw()
		{
			if (name)
				free(name); 
		}
};

TUniqueDraw *TUniqueDraw::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TUniqueDraw *
TUniqueDraw::Append(TUniqueDraw * p)
{
    TUniqueDraw *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TUniqueDraw* TUniqueDraw::SearchByName(char *_name)
{
	for (TUniqueDraw *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}

/** Class - list keeps unique names of report */
class TUniqueReport{
	public:
		TUniqueReport(char * _name, TUniqueReport *_next = NULL)
	{
	name = strdup (_name); 
	}

		/**
		 * Function appends next item to list
		 * @param p pointer to added class
		 * @return pointer to new list (with added p class)
		 */
		TUniqueReport *Append(TUniqueReport * p);

		/**
		 * Function searches list by name
		 * @param _name searched name of param
		 * @param icase when 1 then ignore case sensitive, when 0 no ignore
		 * @return pointer pointed to class with given alt name or NULL when not found
		 */
		TUniqueReport *SearchByName(char *_name) ;

		/**
		 * Function moves pointer to the next class
		 * @return pointer pointed to the next class or NULL when there is no next class
		 */
		TUniqueReport *GetNext();
		TUniqueReport *_next; /** <pointer to next item in list*/ 
		char *name; /** <name (pattern) of param eg. "Sieæ:Sterownik:Temperatura zewnêtrzna" */
		
		~TUniqueReport()
		{
			free(name); 
		}
};

TUniqueReport *TUniqueReport::GetNext(){
	if (_next)
		return (_next);
	return NULL;
}

TUniqueReport *
TUniqueReport::Append(TUniqueReport * p)
{
    TUniqueReport *t = this;
    while (t->_next)
        t = t->_next;

    t->_next = p;
    return p;
}

TUniqueReport* TUniqueReport::SearchByName(char *_name)
{
	for (TUniqueReport *t = this; t; t = t->_next){
			if (!strcmp(t->name,_name) ){
				return t;
			}	
	}
	return NULL;
}


/**
 * main class program
 *
 */ 
class TUnifier: public TProgramConfig{
	public:
	TUnifier():TProgramConfig(){
			dict_param = NULL;
			dict_report = NULL;
			dict_draw = NULL;
			unique_param = NULL;
			ptr_unique_param = NULL;
			unique_draw = NULL;
			ptr_unique_draw = NULL;
			unique_report = NULL;
		//	ptr_unique_report = NULL;
			loaded = false;
	};
	int LoadAlt();
	int LoadConfiguration();	
	int ProcessConfiguration();

	/**Function validates duplikates of params and walidaates name of param (is similar to p1:p2:p3) 
	 * @return 1 if ok, 0, if fails
	 * */
	int ValidateConfiguration();

	TDictParam *dict_param; /** params dictionary list (with alternatives) */
	TDictReport *dict_report; /** reports dictionary list (with alternatives) */ 
	TDictDraw *dict_draw; /** draws dictionary list (with alternatives) */ 
	
	/**
	 * Function wrapps strcmp and strcasecmp
	 * @param str1 first string
	 * @param str2 second (another) string
	 * @param icase
	 * @return the same as str(case)cmp
	 */ 
	int MyStrCmp(char * str1, char * str2, char icase);
	bool loaded; /** flag if (params)xml loaded */
	
	~TUnifier(){
		TUniqueParam * tmp_unique_param;
		while (unique_param){
			tmp_unique_param = unique_param;
			unique_param=unique_param->_next;
			delete tmp_unique_param;
		}

		TUniqueDraw * tmp_unique_draw;
		while (unique_draw){
			tmp_unique_draw = unique_draw;
			unique_draw=unique_draw->_next;
			delete tmp_unique_draw;
		}
		

		TUniqueReport * tmp_unique_report;
		while (unique_report){
			tmp_unique_report = unique_report;
			unique_report=unique_report->_next;
			delete tmp_unique_report;
		}

		TDictReport * tmp_dict_report;
		while (dict_report){
			tmp_dict_report = dict_report;
			dict_report=dict_report->_next;
			delete tmp_dict_report;
		}

		TDictDraw * tmp_dict_draw;
		while (dict_draw){
			tmp_dict_draw = dict_draw;
			dict_draw=dict_draw->_next;
			delete tmp_dict_draw;
		}

		TDictParam * tmp_dict_param;
		while (dict_param){
			tmp_dict_param = dict_param;
			dict_param=dict_param->_next;
			delete tmp_dict_param;
		}

		libpar_done();
		if (ipk) delete ipk;
	};
	TSzarpConfig *ipk ;

	private:
		/**
		 * Function checks wheather param is uniqe
		 * @param _name searched unique name
		 * @return 1 is unique, 0 otherwise
		 */ 
		int IsUniqueParam(char *_name);

		/**
		 * Function checks wheather param is uniqe
		 * @param _name searched unique name
		 * @return 1 is unique, 0 otherwise
		 */ 
		int IsUniqueDraw(char *_name);
	
		/**
		 * Function checks wheather param is uniqe
		 * @param _name searched unique name
		 * @return 1 is unique, 0 otherwise
		 */ 
		int IsUniqueReport(char *_name);
	
		TUniqueParam *unique_param; /** <list of unique params */
		TUniqueParam *ptr_unique_param; /** <ptr to list */

		TUniqueDraw *unique_draw; /** <list of unique draws */
		TUniqueDraw *ptr_unique_draw; /** <ptr to list */

		TUniqueReport *unique_report; /** <list of unique reports */ 

		/**
		 * Function process (replaces) params 
		 * and sends, formulas
		 * @return 1 if ok, 0 otherwise
		 */ 
		int ProcessParams();

		/**
		 * Function process (replaces) draws (windows) 
		 * and sends, formulas
		 * @return 1 if ok, 0 otherwise
		 */ 
		int ProcessDraws();
		
		/**
		 * Function process (replaces) short_name && draw_name 
		 * and sends, formulas
		 * @return 1 if ok, 0 otherwise
		 */ 	
		int ProcessShorts();
		
		/**
		 * Function process (replaces) reports
		 * and sends, formulas
		 * @return 1 if ok, 0 otherwise
		 */ 	
		int ProcessReports();

		/**
		 * Function check duplicates of params in configuration
		 * (because similar function in SZARP doesn't work)
		 * @return 1 if ok, 0 otherwise
		 */ 
		int CheckDuplicates() ;
		
		/** 
		 * Function checks for param is send param 
		 * @return 1 if ok, 0 otherwise
		 */
		int IsSendParam(char *name);
	
		/** 
		 * Function checks for param is in formula 
		 */		
		int IsParamInFormula(char *name);
};

int TUnifier::IsParamInFormula(char *name){
	int result = 0;
	if (!ipk){
		printf("ERROR: ipk not loaded!\n");
		return 0;
	}
	CParamsFormulaReplaces cpfr;
	
	for (TParam *ppp = ipk->GetFirstDefined();ppp;ppp=ipk->GetNextParam(ppp)){
		printf("DEFINED %s \n", (char *)ppp->GetFormula());
		if (cpfr.find_param_in_formula((char *)ppp->GetName(), (char *)name, (char *)ppp->GetFormula())){
			result = 1;
		}
	}
	return result;
}

int TUnifier::IsSendParam(char *name){
	int result = 0;
	if (!ipk){
		printf("ERROR: ipk not loaded!\n");
		return 0;
	}

	for (TDevice *ppp = ipk->GetFirstDevice();ppp;ppp=ipk->GetNextDevice(ppp)){
		for (TRadio *rrr = ppp->GetFirstRadio();rrr;rrr=ppp->GetNextRadio(rrr)){
			for (TUnit *sss = rrr->GetFirstUnit();sss;sss=rrr->GetNextUnit(sss)){
				for (TSendParam *ttt = sss->GetFirstSendParam();ttt;ttt=sss->GetNextSendParam(ttt)){

					if (ttt->GetParamName()){
						if (!strcmp(ttt->GetParamName(), name))
							result = 1;
					}
				}	

			}	
		}
	}
	return result;
}  

int TUnifier::IsUniqueParam(char *_name){
		TUniqueParam *tmp_unique_param;
		if (unique_param){
			tmp_unique_param = unique_param;
			if (unique_param->SearchByName(_name))
				return 0;
		}
		if (ptr_unique_param == NULL){
			ptr_unique_param = unique_param = new TUniqueParam(_name);
		}
	    	else{		
			ptr_unique_param = ptr_unique_param->Append (new TUniqueParam(_name));
		}
		return 1;
}

int TUnifier::IsUniqueDraw(char *_name){
		TUniqueDraw *tmp_unique_draw;
		if (unique_draw){
			tmp_unique_draw = unique_draw;
			if (unique_draw->SearchByName(_name))
				return 0;
		}
		if (ptr_unique_draw == NULL){
			ptr_unique_draw = unique_draw = new TUniqueDraw(_name);
		}
	    	else{		
			ptr_unique_draw = ptr_unique_draw->Append (new TUniqueDraw(_name));
		}
		return 1;
}

int TUnifier::LoadAlt(){
	xmlDocPtr doc;  
	TDictParam *ptr_p = NULL;
	TDictReport *ptr_r = NULL;
	TDictDraw *ptr_d = NULL;

	xmlLineNumbersDefault(1);
	char *Alt;
	Alt = GetAlt();
	if (!Alt)
		return 0;

	doc = xmlParseFile(Alt);
	free(Alt);

	if (doc == NULL) {
		return 0;
	}
	
	int ret;
	xmlRelaxNGPtr alt_rng ;
	xmlRelaxNGParserCtxtPtr ctxt;

	ctxt = xmlRelaxNGNewParserCtxt(
                "file:///"INSTALL_PREFIX"/resources/dtd/unifier.rng");
	alt_rng = xmlRelaxNGParse(ctxt);
        if (alt_rng == NULL) {
                printf("Could not load IPK RelaxNG schema from file %s",
                        INSTALL_PREFIX"/resources/dtd/unifier.rng");
                return 0;
        }

	xmlRelaxNGValidCtxtPtr ctxtv;
	ctxtv = xmlRelaxNGNewValidCtxt(alt_rng);
	ret = xmlRelaxNGValidateDoc(ctxtv, doc);
	xmlRelaxNGFreeValidCtxt(ctxtv);

	xmlNodePtr node;
	xmlNodePtr node2;
	xmlNodePtr node3;
	char *str;
	char *str2;
	char *str3;
	for (node = doc->children; node; node = node->next) {
  		if (node->ns==NULL)
			continue;
   		if (strcmp((char *) node->ns->href, UNIFIER_NAMESPACE_STRING) !=0)
			continue;

		 if (strcmp((char *) node->name, "params") !=0)
			continue;
		 
		/* w sekcji params */
		for(node2=node->children;node2;node2=node2->next){
			 if (node2->ns == NULL) continue;
			 if (strcmp((char *) node2->ns->href, UNIFIER_NAMESPACE_STRING) !=0)
				continue;

			 if ( (strcmp((char *) node2->name, "param") !=0) && (strcmp((char *) node2->name, "draw") !=0) && (strcmp((char *) node2->name, "raport") !=0))
				continue;
			 /* only sections param or draw or raport */
			if (strcmp((char *) node2->name, "param") == 0 && (IsParams() || IsShorts()) ){
				str = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("name")));
				if (str==NULL){
					printf("tag param no tag 'name'");
					return 0;
				}
				str2 = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("draw_name")));
//THIS IS OPTIONAL
//				if (str2==NULL){
//					printf("tag param no tag 'draw_name'");
//					return 0;
//				}

				str3 = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("short_name")));

//THIS IS OPTIONAL
//				if (str3==NULL){
//					printf("tag param has no tag short_name");
//					return 0;
//				}


				if (ptr_p == NULL){
						ptr_p = dict_param = new TDictParam(str, str2, str3);
					}
	    				else{
						ptr_p = ptr_p->Append (new TDictParam(str, str2, str3));

	    				}
					free (str3);
					free (str2);
					free (str);
				
				for(node3=node2->children;node3;node3=node3->next){
					if (strcmp((char *) node3->name, "alt") !=0)
						continue;

					str = (char *)fromUTF8( xmlGetProp(node3,
				    	BAD_CAST("name")));

					if (str==NULL){
						printf("tag param->alt has no tag 'name'");
						return 0;
					}
				
					if (ptr_p)
						ptr_p->InsertAltParam(str);
					free (str);
				
				}
			}
			if (strcmp((char *) node2->name, "draw") == 0 && (IsDraws())){
				str = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("title")));
				if (str==NULL){
					printf("tag draw no tag 'title'");
					return 0;
				}
	
				if (ptr_d == NULL){
						ptr_d = dict_draw = new TDictDraw(str);
					}
	    				else{
						ptr_d = ptr_d->Append (new TDictDraw(str));
	    				}
			
				if (str)	
					free (str);
			
				for(node3=node2->children;node3;node3=node3->next){
					if (strcmp((char *) node3->name, "alt") !=0)
						continue;

					str = (char *)fromUTF8( xmlGetProp(node3,
				    	BAD_CAST("title")));

					if (str==NULL){
						printf("tag draw->alt has no tag 'title'");
						return 0;
					}
					if (ptr_d)
						ptr_d->InsertAltDraw(str);
					free (str);
				}
			
			}

			if (strcmp((char *) node2->name, "raport") == 0 && (IsReports())){
				str = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("title")));
				if (str==NULL){
					printf("tag title has no tag 'name'");
					return 0;
				}

				str2 = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("description")));

				str3 = (char *)fromUTF8( xmlGetProp(node2,
				    BAD_CAST("filename")));

				if (ptr_r == NULL){
						ptr_r = dict_report = new TDictReport(str, str2, str3);
					}
	    				else{
						ptr_r = ptr_r->Append (new TDictReport(str, str2, str3));
	    				}
			
				if (str3)
					free(str3);
				if (str2)
					free(str2);
				if (str)	
					free (str);
			
				for(node3=node2->children;node3;node3=node3->next){
					if (strcmp((char *) node3->name, "alt") !=0)
						continue;

					str = (char *)fromUTF8( xmlGetProp(node3,
				    	BAD_CAST("title")));

					if (str==NULL){
						printf("tag draw->alt has no tag 'title'");
						return 0;
					}
					str2 = (char *)fromUTF8( xmlGetProp(node3,
					    	BAD_CAST("description")));
					str3 = (char *)fromUTF8( xmlGetProp(node3,
					    	BAD_CAST("filename")));
					if (ptr_r)
						ptr_r->InsertAltReport(str, str2, str3);

					if (str3)
						free (str3);
					if (str2)
						free (str2);
					if (str)
						free (str);
				}
			
			}

		}
	}
	xmlFreeDoc(doc);
	return CheckDuplicates() ;
	
}

int TUnifier::CheckDuplicates(){
	TDictParam *tmp_dict_param;
	TAltDictParam *tmp_alt_dict_param;
	TUniqueParam * tmp_unique_param;
	for (tmp_dict_param = dict_param;tmp_dict_param;tmp_dict_param=tmp_dict_param->GetNext()){	
		if (!IsUniqueParam(tmp_dict_param->name)){
			while (unique_param){
				tmp_unique_param = unique_param;
				unique_param=unique_param->_next;
				delete tmp_unique_param;
			}
			printf("ERROR: file %s has the same param name: %s\n", m_Alt, tmp_dict_param->name);
			return 0;
		}
		if (tmp_dict_param->alt_dict_params != NULL){ 
			for (tmp_alt_dict_param = tmp_dict_param->alt_dict_params; tmp_alt_dict_param;tmp_alt_dict_param=tmp_alt_dict_param->GetNext()){
			if (!IsUniqueParam(tmp_alt_dict_param->alt_name)){
				while (unique_param){
					tmp_unique_param = unique_param;
					unique_param=unique_param->_next;
					delete tmp_unique_param;
				}
				printf("ERROR: file %s has the same alt name: %s\n", m_Alt, tmp_alt_dict_param->alt_name);
				return 0;
			}
		}
	}

	}

	while (unique_param){
		tmp_unique_param = unique_param;
		unique_param=unique_param->_next;
		delete tmp_unique_param;
	}
	unique_param = NULL;
	ptr_unique_param = NULL;

return 1;
}

int TUnifier::MyStrCmp(char * str1, char * str2,char icase){
	if (icase)
		return strcasecmp(str1,str2);
	return(strcmp(str1,str2));
}

int TUnifier::LoadConfiguration()
{
	ipk = new TSzarpConfig();
	if (path)
		ipk->loadXML(path);
	else{
		delete ipk;
		return 0;
	}

	return ValidateConfiguration();
}

int TUnifier::ProcessParams(){
	TDictParam *tmp_dict_param;
	TAltDictParam *tmp_alt_dict_param;
	TXmlReplace *xml_replace = NULL;
	char result = 0;
	bool was = false;
	for (TParam *ppp = ipk->GetFirstParam();ppp;ppp=ipk->GetNextParam(ppp)){
	result = 0;
	for (tmp_dict_param = dict_param;tmp_dict_param;tmp_dict_param=tmp_dict_param->GetNext()){
		if (!MyStrCmp (tmp_dict_param->name, (char *)ppp->GetName(),IsICase())){ /* pattern is peaceable */
			if (tmp_dict_param->draw_name){
				if (ppp->GetDrawName()){
					if (strcmp(tmp_dict_param->draw_name, ppp->GetDrawName())){
						if (loaded==false){
									xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
								loaded = true;
							}else{
									xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							}
							
							if (!IsCheck())
								xml_replace->ReplaceDrawName(tmp_dict_param->draw_name);
							if (IsVerbose()) printf("REPLACE 'draw_name': '%s' '%s' (%s)\n", (char *)ppp->GetDrawName(), (char *)tmp_dict_param->draw_name, (char *)ppp->GetName());
							if (xml_replace)
								delete xml_replace;
					}
				}
			}
			if (tmp_dict_param->short_name){
				if (ppp->GetShortName()){ 
					if (strcmp(tmp_dict_param->short_name, ppp->GetShortName())){
							if (loaded==false){
									xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
								loaded = true;
							}else{
									xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							}
							if (!IsCheck())
								xml_replace->ReplaceShortName(tmp_dict_param->short_name);
							if (IsVerbose()) printf("REPLACE 'short_name': '%s' '%s' (%s)\n", (char *)ppp->GetShortName(), tmp_dict_param->short_name, (char *)ppp->GetName());
						if (xml_replace)
							delete xml_replace;
					}
				}
			}
			result = 1;
		}else{ /* pattern is not peaceable */
if (tmp_dict_param->alt_dict_params != NULL){ /* there are alternate names */
	was = false;
	for (tmp_alt_dict_param = tmp_dict_param->alt_dict_params; tmp_alt_dict_param;tmp_alt_dict_param=tmp_alt_dict_param->GetNext()){
	if (!MyStrCmp(tmp_alt_dict_param->alt_name, (char *)ppp->GetName(),IsICase()) && !was){
		was = true;
		result = 2;	
		if (ppp->GetDrawName() && tmp_dict_param->draw_name){
			if (strcmp(tmp_dict_param->draw_name, ppp->GetDrawName())){
					if (loaded==false){
							xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
						loaded = true;
					}else{
							xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
					}
					if (!IsCheck())
						xml_replace->ReplaceDrawName(tmp_dict_param->draw_name);
					if (IsVerbose()) printf("REPLACE 'draw_name': '%s' '%s' (%s)\n", (char *)ppp->GetDrawName(), tmp_dict_param->draw_name, ppp->GetName());
					if (xml_replace)
						delete xml_replace;
			}
		}
		
		if (tmp_dict_param->short_name){
			if (ppp->GetShortName()){
				if (strcmp(tmp_dict_param->short_name, ppp->GetShortName())){
						if (loaded==false){
								xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							loaded = true;
						}else{
								xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
						}
						if (!IsCheck())
							xml_replace->ReplaceShortName(tmp_dict_param->short_name);
						if (IsVerbose()) printf("REPLACE 'short_name': '%s' '%s' (%s)\n", (char *)ppp->GetShortName(), tmp_dict_param->short_name, ppp->GetName());
						if (xml_replace)
							delete xml_replace;
					}	
			}

		
		}
/* replacowanie nazwy parametru */
	
			if (loaded==false){
					xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
					loaded = true;
			}else{
					xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
			}
				if (!IsCheck())
					xml_replace->ReplaceParam(tmp_dict_param->name);
				if (IsVerbose()) printf("REPLACE 'param': '%s' '%s' \n", (char *)ppp->GetName(), tmp_dict_param->name);

				if (xml_replace)
					delete xml_replace;

/* replacowanie parametrow w senderze */

		if (IsSendParam((char *)ppp->GetName())){
				if (loaded==false){
						xml_replace = new TXmlReplace((char *)path, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
					loaded = true;
				}else{
						xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
				}
				if (!IsCheck())
					xml_replace->ReplaceSendParam(tmp_dict_param->name);
				if (IsVerbose()) printf("REPLACE 'send param': %s %s\n", (char *)ppp->GetName(), tmp_dict_param->name);

				if (xml_replace)
					delete xml_replace;
		}

/* replacowanie parametrow w defined i drawdefinable */
	CParamsFormulaReplaces cpfr;
	TSzarpConfig *temp_ipk ;
	temp_ipk = new TSzarpConfig();
	if (loaded==false){
		if (path)
			temp_ipk->loadXML(path);
	}else{
		if (m_Output)
			temp_ipk->loadXML(m_Output);
	}
	


	for (TParam *xxx = temp_ipk->GetFirstDefined();xxx;xxx=temp_ipk->GetNextParam(xxx)){
		if (xxx->GetFormula()){
			if (cpfr.find_param_in_formula((char *)xxx->GetName(), (char *)ppp->GetName(), (char *)xxx->GetFormula())){
			char *processed_formula;	
			processed_formula = cpfr.process_formula((char *)xxx->GetName(), (char *) ppp->GetName(), (char *)tmp_dict_param->name, (char *)xxx->GetFormula()) ;

			if (loaded==false){
						xml_replace = new TXmlReplace((char *)path, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)xxx->GetName());
					loaded = true;
				}else{
						xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)xxx->GetName());

				}
			if (!IsCheck())
				xml_replace->ReplaceFormula((char *)processed_formula);
			if (IsVerbose()) printf("REPLACE 'formula': '%s' '%s' OF PARAM (%s) IN PARAM (%s) \n", (char *)xxx->GetFormula(), processed_formula, ppp->GetName(), xxx->GetName());

			free (processed_formula);
			}	
		}
	}
	if (temp_ipk)
		delete temp_ipk;

/* copying data */
	if (ppp->IsInBase() && !ppp->IsDefinable()){//Kopiujemy tylko dane w bazie i nie drawdefinable
		char *p1 = strdup(ppp->GetName());
		char *p2 = strdup(tmp_dict_param->name);
		latin2szb(p1);
		latin2szb(p2);
		if (strcmp(p1, p2)){ //jezeli parametry przed zmiana nazwy i po zmianie nazwy sa takie same to po co kopiowac
			string source_dir(dir);
			source_dir.append(DIR_SEPARATOR);
			source_dir.append(p1);
			string dest_dir(dir);
			dest_dir.append(DIR_SEPARATOR);
			dest_dir.append(p2);
			FileActions fa;	
			CStrReplaces sr;
			string my_prefix(dir);


	if (!IsCheck())
		if (!fa.create_szbase_dirs((char *)my_prefix.c_str(), (char *)p2)){
			printf("Error. Fail to create szbase dirs '%s' \n", source_dir.c_str());
			return 0;
		}

	if (!IsCheck())
		if (!fa.dir_copy((char *)source_dir.c_str(), (char *)dest_dir.c_str())){
			printf("Error. Fail to copy directory 'from': '%s' 'to': '%s' \n", (char *)source_dir.c_str(), (char *)dest_dir.c_str());
			return 0;
		}

	if (!IsCheck())
		if (!fa.dir_remove_files((char *)source_dir.c_str())){
			printf("Error. Fail to remove files 'from': '%s' \n", source_dir.c_str());
			return 0;
		}

	if (!IsCheck())
		if (!fa.remove_empty_szbase_dirs((char *)my_prefix.c_str(), (char *)p1)){
			printf("Error. Fail to remove empty szbase dir '%s' \n", source_dir.c_str());
			return 0;
		}
	
	if (IsVerbose()) printf("COPY 'from': '%s' 'to': '%s' \n", source_dir.c_str(), dest_dir.c_str());
		free(p2);
		free(p1);
		}
	}



		}//strcmp
	}//for
	}//!NULL
	}//else
	}//alt for


	if (result==0){
		printf("Param name: name=\"%s\" draw_name=\"%s\" short_name=\"%s\" not found in Alt file.\n",(char *)ppp->GetName(), ppp->GetDrawName(), ppp->GetShortName());	
	}

	/*koniec*/
	}//main for
	return 1;
}


//					if (was==false){
//						if (loaded==false){
//							xml_replace = new TXmlReplace((char *)path, (char *)m_Output,IPK_NAMESPACE_STRING, "NULL");
//							loaded = true;
//						}else{
//							xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,IPK_NAMESPACE_STRING, "NULL");
//						}
//
//				//			xml_replace->ReplaceParamTitle(s, (tmp_dict_param->name));
//							if (IsVerbose()) printf("REPLACE: %s %s\n", (char *)ppp->GetName(), tmp_dict_param->name);
//
//							delete xml_replace;
//							was = true;
//						}	}
//						}
//

int TUnifier::ProcessShorts(){
	TDictParam *tmp_dict_param;
	TAltDictParam *tmp_alt_dict_param;
	TXmlReplace *xml_replace = NULL;
	char result = 0;
	bool was = false;
	for (TParam *ppp = ipk->GetFirstParam();ppp;ppp=ipk->GetNextParam(ppp)){
	/*begin*/
	result = 0;
	for (tmp_dict_param = dict_param;tmp_dict_param;tmp_dict_param=tmp_dict_param->GetNext()){
		if (!MyStrCmp (tmp_dict_param->name, (char *)ppp->GetName(),IsICase())){ /* pattern is peaceable */
			if (tmp_dict_param->draw_name){
				if (ppp->GetDrawName()){
					if (strcmp(tmp_dict_param->draw_name, ppp->GetDrawName())){
						if (loaded==false){
									xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
								loaded = true;
							}else{
									xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							}

							if(!IsCheck())
								xml_replace->ReplaceDrawName(tmp_dict_param->draw_name);
							if (IsVerbose()) printf("REPLACE 'draw_name': '%s' '%s' (%s)\n", (char *)ppp->GetDrawName(), (char *)tmp_dict_param->draw_name, (char *)ppp->GetName());
							if (xml_replace)
								delete xml_replace;
					}
				}
			}
			if (tmp_dict_param->short_name){
				if (ppp->GetShortName()){
					if (strcmp(tmp_dict_param->short_name, ppp->GetShortName())){
							if (loaded==false){
									xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
								loaded = true;
							}else{
									xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							}
								
							if(!IsCheck())
								xml_replace->ReplaceShortName(tmp_dict_param->short_name);
							if (IsVerbose()) printf("REPLACE 'short_name': '%s' '%s' (%s)\n", (char *)ppp->GetShortName(), tmp_dict_param->short_name, (char *)ppp->GetName());
						if (xml_replace)
							delete xml_replace;
					}
				}
			}
			result = 1;
		}else{ /* pattern is not peaceable */
if (tmp_dict_param->alt_dict_params != NULL){ /* there are alternate names */
	was = false;
	for (tmp_alt_dict_param = tmp_dict_param->alt_dict_params; tmp_alt_dict_param;tmp_alt_dict_param=tmp_alt_dict_param->GetNext()){
	if (!MyStrCmp(tmp_alt_dict_param->alt_name, (char *)ppp->GetName(),IsICase()) && !was){
		was = true;
		result = 2;	
		if (ppp->GetDrawName() && tmp_dict_param->draw_name){
			if (strcmp(tmp_dict_param->draw_name, ppp->GetDrawName())){
					if (loaded==false){
							xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
						loaded = true;
					}else{
							xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
					}
					if(!IsCheck())
						xml_replace->ReplaceDrawName(tmp_dict_param->draw_name);
					if (IsVerbose()) printf("REPLACE 'draw_name': '%s' '%s' (%s)\n", (char *)ppp->GetDrawName(), tmp_dict_param->draw_name, ppp->GetName());
					if (xml_replace)
						delete xml_replace;
			}
		}
		
		if (tmp_dict_param->short_name){
			if (ppp->GetShortName()){
				if (strcmp(tmp_dict_param->short_name, ppp->GetShortName())){
						if (loaded==false){
								xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
							loaded = true;
						}else{
								xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
						}

						if(!IsCheck())
							xml_replace->ReplaceShortName(tmp_dict_param->short_name);
						if (IsVerbose()) printf("REPLACE 'short_name': '%s' '%s' (%s)\n", (char *)ppp->GetShortName(), tmp_dict_param->short_name, ppp->GetName());
						if (xml_replace)
							delete xml_replace;
					}	
			}
		}

		if (IsSendParam((char *)ppp->GetName())){
				if (loaded==false){
						xml_replace = new TXmlReplace((char *)path, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
					loaded = true;
				}else{
						xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output, (char *)IPK_NAMESPACE_STRING, (char *)ppp->GetName());
				}

				if(!IsCheck())
					xml_replace->ReplaceSendParam(tmp_dict_param->short_name);
				if (IsVerbose()) printf("REPLACE 'send param': %s %s\n", (char *)ppp->GetShortName(), tmp_dict_param->short_name);

				delete xml_replace;
		}
		
		}//strcmp
	}//for
	}//!NULL
	}//else
	}//alt for


	if (result==0){
		printf("Param name: name=\"%s\" draw_name=\"%s\" short_name=\"%s\" not found in Alt file.\n",(char *)ppp->GetName(), ppp->GetDrawName(), ppp->GetShortName());	
	}

	/*end*/
	}//main for
	return 1;
}

int TUnifier::ProcessDraws(){
	char *s;
	TDictDraw *tmp_dict_draw;
	TAltDictDraw *tmp_alt_dict_draw;
	TXmlReplace *xml_replace;
	char result = 0;
	bool was = false;

	for (TParam *ppp = ipk->GetFirstParam();ppp;ppp=ipk->GetNextParam(ppp)){
		for (TDraw *ddd=ppp->GetDraws();ddd;ddd=ddd->GetNext()){
			s = strdup(ddd->GetWindow());
			if (s){
				if (IsUniqueDraw(s)){
					result = 0;

					for (tmp_dict_draw = dict_draw;tmp_dict_draw;tmp_dict_draw=tmp_dict_draw->GetNext()){


						if (!MyStrCmp (tmp_dict_draw->name, s,IsICase())){ /* pattern is peaceable */
							result = 1;	
						}else{ /* pattern is not peaceable */
							if (tmp_dict_draw->alt_dict_draws != NULL){ /* there are alternate names */
								was = false;
								for (tmp_alt_dict_draw = tmp_dict_draw->alt_dict_draws; tmp_alt_dict_draw;tmp_alt_dict_draw=tmp_alt_dict_draw->GetNext()){
									if (!MyStrCmp(tmp_alt_dict_draw->alt_name,s,IsICase())){
											result =2;	

											if (was==false){
											

												if (loaded==false){
													xml_replace = new TXmlReplace((char *)path, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)"NULL");
													loaded = true;
												}else{
													xml_replace = new TXmlReplace((char *)m_Output, (char *)m_Output,(char *)IPK_NAMESPACE_STRING, (char *)"NULL");
												}
												xml_replace->ReplaceDrawTitle(s, (tmp_dict_draw->name));
												if (IsVerbose()) printf("REPLACE: %s %s\n", s, tmp_dict_draw->name);

												delete xml_replace;
												was = true;
											}





																		}
								}






							}
						}	
					}
					if (result==0){
							printf("Window name: '%s' not found in Alt file.\n",s);	
					}

			}
			free(s);	
		}
	}
}
	return 1;
}

int TUnifier::ProcessReports(){

	return 1;
}

int TUnifier::ProcessConfiguration()
{
	if (IsParams()) ProcessParams();
	if (IsDraws()) ProcessDraws();
	if (IsShorts()) ProcessShorts();
	if (IsReports()) ProcessReports();
	return 1;
}

int TUnifier::ValidateConfiguration(){
	const char *name;
	char flag = 0;
	unsigned int i;
	for (TParam *ppp = ipk->GetFirstParam();ppp;ppp=ipk->GetNextParam(ppp)){
		if (!IsUniqueParam((char *)ppp->GetName())){
			printf("Fatal error. Param with name '%s' is not uniqe \n", ppp->GetName());	
			return 0;
		}
		flag = 0;	
		name = ppp->GetName(); 
		for (i=0; i<strlen(name); i++){
			if (name[i]==':'){
				flag++;	
			}
		}
		if (flag != 2){
			printf("Fatal error. Param with name '%s' is not represented by 'param 1:param 2:param 3' \n", ppp->GetName());	
			return 0;
		}
	}


	return 1;
}

int main (int argc, char *argv[]){
	TUnifier* MyUnifier;
	MyUnifier = new TUnifier();
	if (!MyUnifier->ParseCommandLine(argc, argv)){
		cout <<"Error while parsing command line."<<endl;
		delete MyUnifier;
		return 1;	
	}
	if (!MyUnifier->LoadAlt()){
		cout <<"Error. Failed to load alt file."<<endl;
		delete MyUnifier;
		return 1;
	}
	if (!MyUnifier->LoadConfiguration()){
		cout <<"Error. Failed to load configuration (params.xml) file."<<endl;
		delete MyUnifier;
		return 1;
	}
	if (!MyUnifier->ProcessConfiguration()){
		cout <<"Error. Failed to process data."<<endl;
		delete MyUnifier;
		return 1;
	}
	if (MyUnifier->IsVerbose())
		cout <<"That's all folks!"<<endl;
	delete MyUnifier;
	return 0;
}
