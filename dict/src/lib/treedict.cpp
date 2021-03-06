/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Implementation of class to work with GtkTree
 * based StarDict's dictionaries
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include "file-utils.h"
#include "utils.h"

#include "treedict.h"

GtkTreeStore *TreeDict::model=NULL;

TreeDict::TreeDict()
{
	if (model)
		return;

	// It is said G_TYPE_UINT will always be 32 bit.
	// see http://bugzilla.gnome.org/show_bug.cgi?id=337966
	model = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT); //word, offset, size
}

bool TreeDict::load(const std::string& ifofilename)
{
	gulong tdxfilesize;
	if (!load_ifofile(ifofilename, &tdxfilesize))
		return false;

	if(!DictBase::load(ifofilename.substr(0, ifofilename.length()-sizeof(".ifo")+1),
		"dict"))
		return false;

	std::string fullfilename(ifofilename);
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "tdx.gz");

	gchar *buffer= NULL;
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		gzFile in;
		in = gzopen(fullfilename.c_str(),"rb");
		if (in == NULL) {
			//g_print("Open file %s failed!\n",idxfilename);
			return false;
		}

		buffer = (gchar *)g_malloc(tdxfilesize);

		gulong len;
		len = gzread(in, buffer, tdxfilesize);
		if (len < 0) {
			g_free(buffer);
			return false;
		}
		gzclose(in);
		if (len != tdxfilesize) {
			g_free(buffer);
			return false;
		}
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		FILE *file;
		if (!(file = fopen (fullfilename.c_str(), "rb"))) {
			//g_print("Open file %s failed!\n",fullfilename);
			return false;
		}
		buffer = (gchar *)g_malloc(tdxfilesize);
		size_t read_len;
		read_len = fread(buffer, tdxfilesize, 1, file);
		fclose(file);
		if (read_len!=1) {
			g_free(buffer);
			return false;
		}
	}

	gchar *tmp_buffer = buffer;
	load_model(&tmp_buffer, NULL, 1); // tmp_buffer will be changed.
	g_free(buffer);
	return true;
}

bool TreeDict::load_ifofile(const std::string& ifofilename, gulong *tdxfilesize)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(ifofilename, DictInfoType_TreeDict))
		return false;

	*tdxfilesize = dict_info.get_index_file_size();
	sametypesequence=dict_info.get_sametypesequence();

	return true;
}

void TreeDict::load_model(gchar **buffer, GtkTreeIter *parent, guint32 count)
{
	GtkTreeIter iter;
	gchar *p1;
	guint32 offset, size, subentry_count;

	for (guint32 i=0; i< count; i++) {
		p1 = *buffer + strlen(*buffer) +1;
		offset = g_ntohl(get_uint32(p1));
		p1 += sizeof(guint32);
		size = g_ntohl(get_uint32(p1));
		p1 += sizeof(guint32);
		subentry_count = g_ntohl(get_uint32(p1));
		p1 += sizeof(guint32);
		gtk_tree_store_append(model, &iter, parent);
		gtk_tree_store_set(model, &iter, 0, *buffer, 1, offset, 2, size, -1);
		*buffer = p1;
		if (subentry_count)
			load_model(buffer, &iter, subentry_count);
	}
}


/**************************************************/
TreeDicts::TreeDicts()
{
}

TreeDicts::~TreeDicts()
{
	for (std::vector<TreeDict *>::iterator it=oTreeDict.begin();
	     it!=oTreeDict.end(); ++it)
		delete *it;
}

void TreeDicts::load_dict(const std::string& url)
{
	TreeDict *lib = new TreeDict;
	if (lib->load(url))
		oTreeDict.push_back(lib);
	else
		delete lib;
}

class TreeDictLoader {
public:
	TreeDictLoader(TreeDicts& td_) : td(td_) {}
	void operator()(const std::string& url, bool disable) {
		if (!disable)
			td.load_dict(url);
	}
private:
	TreeDicts& td;
};

GtkTreeStore* TreeDicts::Load(const strlist_t& tree_dicts_dirs,
			      const strlist_t& order_list,
			      const strlist_t& disable_list)
{
	TreeDictLoader load(*this);
	for_each_file(tree_dicts_dirs, ".ifo", order_list, disable_list, load);

	return TreeDict::get_model();
}

gchar* TreeDicts::poGetWordData(guint32 offset, guint32 size, int iTreeDict)
{
	return oTreeDict[iTreeDict]->GetWordData(offset, size);
}
