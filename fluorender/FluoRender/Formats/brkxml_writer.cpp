#include "brkxml_writer.h"
#include "compatibility.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>

BRKXMLWriter::BRKXMLWriter()
{
	tinyxml2::XMLDeclaration* decl = m_md_doc.NewDeclaration();

	m_md_doc.InsertEndChild(decl);

	tinyxml2::XMLElement* root = m_md_doc.NewElement("Metadata");
	m_md_doc.InsertEndChild(root);
}

BRKXMLWriter::~BRKXMLWriter()
{
}

void BRKXMLWriter::SetData(Nrrd* data)
{
}

void BRKXMLWriter::SetSpacings(double spcx, double spcy, double spcz)
{
}

void BRKXMLWriter::SetCompression(bool value)
{
}

void BRKXMLWriter::Save(wstring filename, int mode)
{
}

//save only a vvd_xml file with metadata
void BRKXMLWriter::SaveVVDXML_Metadata(wstring filepath, tinyxml2::XMLDocument *vvd, tinyxml2::XMLDocument *metadata)
{
	tinyxml2::XMLDocument *md_doc = metadata;

	if (!md_doc)
		md_doc = &m_md_doc;

	if (!md_doc || !vvd) return;

	tinyxml2::XMLElement *md_node = NULL;

	tinyxml2::XMLElement *md_root = md_doc->RootElement();
	if (!md_root || strcmp(md_root->Name(), "Metadata"))
		return;

	tinyxml2::XMLElement *root = vvd->RootElement();
	if (!root || strcmp(root->Name(), "BRK"))
		return;

	tinyxml2::XMLElement *child = root->FirstChildElement();
	while (child)
	{
		if (child->Name())
		{
			if (strcmp(child->Name(), "Metadata") == 0)
			{
				child->DeleteChildren();
				md_node = child;
			}
		}
		child = child->NextSiblingElement();
	}

	if (!md_node)
		root->InsertEndChild(md_root);
	else
	{
		tinyxml2::XMLElement *child = md_node->FirstChildElement();
		map<string, tinyxml2::XMLElement*> clist;
		while (child)
		{
			if (child->Name())
				clist[child->Name()] = child;
			child = child->NextSiblingElement();
		}

		tinyxml2::XMLElement *mchild = md_root->FirstChildElement();
		while (mchild)
		{
			if (mchild->Name())
			{
				auto ite = clist.find(mchild->Name());
				if (ite != clist.end())
					md_root->DeleteChild(clist[mchild->Name()]);
				md_root->InsertEndChild(mchild);
			}
			mchild = mchild->NextSiblingElement();
		}
	}

	FILE *fp = NULL;
	fp = WFOPEN(&fp, filepath.c_str(), L"w");
	if (fp) vvd->SaveFile(fp);
	fclose(fp);

	return;
}

//save only a vvd_xml file with external metadata
void BRKXMLWriter::SaveVVDXML_ExternalMetadata(wstring filepath, tinyxml2::XMLDocument *vvd, tinyxml2::XMLDocument *metadata)
{
	tinyxml2::XMLDocument *md_doc = metadata;

#ifdef _WIN32
	wchar_t slash = L'\\';
#else
	wchar_t slash = L'/';
#endif
	//separate path and name
	size_t pos = filepath.find_last_of(slash);
	wstring dir = filepath.substr(0, pos+1);
	wstring name = filepath.substr(pos+1);
	pos = name.find_last_of('.');
	wstring mname;
	if (pos != wstring::npos)
		mname = name.substr(0, pos+1) + ".xml";
	else
		mname = name + ".xml";

	if (!md_doc)
		md_doc = &m_md_doc;

	if (!md_doc || !vvd) return;

	tinyxml2::XMLElement *md_node = NULL;

	tinyxml2::XMLElement *md_root = md_doc->RootElement();
	if (!md_root || strcmp(md_root->Name(), "Metadata"))
		return;

	tinyxml2::XMLElement *root = vvd->RootElement();
	if (!root || strcmp(root->Name(), "BRK"))
		return;

	string ex_metadata_paths;

	if (root->Attribute("exMetadataPath"))
	{
		ex_metadata_paths = root->Attribute("exMetadataPath");
		size_t ppos = ex_metadata_paths.find(ws2s(mname));
		if (ppos == string::npos)
		{
			ex_metadata_paths = string("") + root->Attribute("exMetadataPath") + "," + ws2s(mname).c_str();
			root->SetAttribute("exMetadataPath", ex_metadata_paths.c_str());
		}
	}
	else
		root->SetAttribute("exMetadataPath", ws2s(mname).c_str());

	FILE *fp = NULL;
	fp = WFOPEN(&fp, filepath.c_str(), L"w");
	if (fp) vvd->SaveFile(fp);
	fclose(fp);

	FILE *mfp = NULL;
	mfp = WFOPEN(&mfp, (dir+mname).c_str(), L"w");
	if (mfp) md_doc->SaveFile(mfp);
	fclose(mfp);

	return;
}

void BRKXMLWriter::SaveMetadataXML(wstring filepath, tinyxml2::XMLDocument *metadata)
{
	tinyxml2::XMLDocument *md_doc = metadata;

	if (!md_doc)
		md_doc = &m_md_doc;

	FILE *mfp = NULL;
	mfp = WFOPEN(&mfp, filepath.c_str(), L"w");
	if (mfp) md_doc->SaveFile(mfp);
	fclose(mfp);
}

void BRKXMLWriter::buildROITreeXML(const boost::property_tree::wptree& tree, const map<int,vector<int>>& palette, const wstring& parent, tinyxml2::XMLElement *lvNode)
{
	if (!lvNode)
	{
		tinyxml2::XMLElement *root = m_md_doc.RootElement();
		if (!root || strcmp(root->Name(), "Metadata"))
			return;
		tinyxml2::XMLElement *child = root->FirstChildElement();
		while (child)
		{
			if (child->Name())
			{
				if (strcmp(child->Name(), "ROI_Tree") == 0)
				{
					child->DeleteChildren();
					lvNode = child;
				}
			}
			child = child->NextSiblingElement();
		}
		if (!lvNode)
		{
			lvNode = m_md_doc.NewElement("ROI_Tree");
			root->InsertEndChild(lvNode);
		}
	}

	if (!lvNode) return;

	for (auto child = tree.begin(); child != tree.end(); ++child)
	{
		wstring c_path = (parent.empty() ? L"" : parent + L".") + child->first;
		wstring strid = child->first;
		wstring name;
		tinyxml2::XMLElement* elem = NULL;
		int id = -1;
		try
		{
			id = boost::lexical_cast<int>(strid);
			if (id != -1)
			{
				if (const auto val = tree.get_optional<wstring>(child->first))
				{
					if (id >= 0)
						elem = m_md_doc.NewElement("ROI");
					else if (id < -1)
						elem = m_md_doc.NewElement("Group");
					elem->SetAttribute("name", ws2s(*val).c_str());
					elem->SetAttribute("id", id);
					auto ite = palette.find(id);
					if (ite != palette.end() && ite->second.size()==4)
					{
						elem->SetAttribute("r", ite->second[1]);
						elem->SetAttribute("g", ite->second[2]);
						elem->SetAttribute("b", ite->second[3]);
					}
					lvNode->InsertEndChild(elem);
					buildROITreeXML(child->second, palette, c_path, elem);
				}
			}
		}
		catch (boost::bad_lexical_cast e)
		{
			cerr << "BRKXMLWriter::buildROITreeXML(const wptree& tree, const wstring& parent, XMLElement *lvNode): bad_lexical_cast" << endl;
		}
	}
}

void BRKXMLWriter::AddROITreeToMetadata(wstring roi_tree)
{
	wistringstream wiss(roi_tree);
	boost::property_tree::wptree tree;
	map<int, vector<int>> cols;

	while (1)
	{
		wstring path, name, strcol;

		if (!getline(wiss, path))
			break;
		if (!getline(wiss, name))
			break;
		if (!getline(wiss, strcol))
			break;

		vector<int> col;
		int ival;
		std::wistringstream ls(strcol);
		while (ls >> ival)
			col.push_back(ival);

		if (col.size() < 4)
			break;

		tree.add(path, name);
		if (col[0] > 0)
			cols[col[0]] = col; 
	}
	buildROITreeXML(tree, cols);
}
