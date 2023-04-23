#include <iostream>
#include <string>
#include <sstream>

using namespace std;

struct Tree {
  int val;
  Tree *left;
  Tree *right;
  Tree():
    val(0),
    left(0),
    right(0)
  {}
  void print();
};

void Tree::print()
{
  
  if (left) {
    left->print();
  }
  cout<<val<<" ";
  if (right) {
    right->print();
  }
}

class Sol {
public:
  Tree *makeTree();
  void deserialize(string &s);
  string serialize(Tree *r);
private:
  Tree *makeNode(int v);
  Tree *helper_d(stringstream &ss);
  void helper_s(Tree *t, string &s);
};

Tree *Sol::makeTree()
{
  Tree *root = makeNode(1);
  root->right = makeNode(2);
  root->right->right = makeNode(3);
  return root;
}

Tree *Sol::makeNode(int v)
{
  Tree *node = new Tree;
  node->val = v;
  return node;
}

Tree * Sol::helper_d(stringstream &ss)
{
  string str;
  if (getline(ss, str, ',')) {
    if (str.compare("#") == 0) {
      return 0;
    }
    Tree *node = makeNode(stoi(str));
    node->left = helper_d(ss);
    node->right = helper_d(ss);
    return node;
  }
  return 0;
}

void Sol::helper_s(Tree *r, string &s)
{
  if (r) {
    std::string val_s = to_string(r->val);
    if (s.length() != 0) {
      s.append(",");
    }
    s.append(val_s);
    helper_s(r->left, s);
    helper_s(r->right, s);
  } else {
    s.append(",");
    s.append("#");
  }
}

string Sol::serialize(Tree *r)
{
  string s;
  helper_s(r, s);
  return s;
}

void Sol::deserialize(string &s)
{
  std::stringstream ss(s);
  Tree *root = helper_d(ss);
  root->print();
  //string s1 = serialize(root);
  //cout<<endl<<"After serialization.. "<<endl;
  //cout<<s1<<endl;;
}

int main()
{
  Sol obj;
  std::string s("1,#,2,#,3,4,#,#,#"); 
  //obj.deserialize(s);
  Tree *root = obj.makeTree();
  string s1 = obj.serialize(root);
  cout<<endl<<"After serialization.. "<<endl;
  cout<<s1<<endl;;
  return 0;
}
