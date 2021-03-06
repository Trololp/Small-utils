// TREE traversal testings

#include "stdafx.h"
#include <stdlib.h>
#include <vector>
#pragma warning(disable : 4996)

struct Tree_node {
	std::string name;
	Tree_node* left;
	Tree_node* right;
};



Tree_node* build_tree(const char* descr, size_t descr_size)
{
	std::string node_name;

	Tree_node* left = nullptr;
	Tree_node* right = nullptr;

	for (int i = 0; i < descr_size; i++)
	{
		if (descr[i] == '(') {
			i++;
			if (descr[i] == ')') {
				break;
			}
			else if (descr[i] == ',')
			{
				right = build_tree(descr + i + 1, descr_size - i - 1);
				break;
			}

			left = build_tree(descr + i, descr_size - i);
			
			while (descr[i] != '(') i++;
			i++;

			int inside_v = 1;

			while (inside_v) {
					if (descr[i] == '(') inside_v++;
					if (descr[i] == ')') inside_v--;
					i++;
			}

			if (descr[i] == ',') {
				right = build_tree(descr + i + 1, descr_size - i - 1);
			}
			break;
		}
		node_name.push_back(descr[i]);
	}
	node_name.push_back(0);

	Tree_node* new_node = new Tree_node;
	new_node->name = node_name;
	new_node->left = left;
	new_node->right = right;

	return new_node;
}

void tree_traverse_inorder(Tree_node* tree_node) {
	if (!tree_node) return;

	printf("%s ", tree_node->name.data());
	tree_traverse_inorder(tree_node->left);
	tree_traverse_inorder(tree_node->right);
}

void tree_traverse_midorder(Tree_node* tree_node) {
	if (!tree_node) return;

	tree_traverse_midorder(tree_node->left);
	printf("%s ", tree_node->name.data());
	tree_traverse_midorder(tree_node->right);
}

/* Function to traverse the binary tree without recursion
and without stack */
void MorrisTraversal(struct Tree_node* root)
{
	struct Tree_node *current, *pre;

	if (root == NULL)
		return;

	current = root;
	while (current != NULL) {

		if (current->left == NULL) {
			printf("%s ", current->name.data());
			current = current->right;
		}
		else {

			/* Find the inorder predecessor of current */
			pre = current->left;
			while (pre->right != NULL
				&& pre->right != current)
				pre = pre->right;

			/* Make current as the right child of its
			inorder predecessor */
			if (pre->right == NULL) {
				pre->right = current;
				current = current->left;
			}

			/* Revert the changes made in the 'if' part to
			restore the original tree i.e., fix the right
			child of predecessor */
			else {
				pre->right = NULL;
				printf("%s ", current->name.data());
				current = current->right;
			} /* End of if condition pre->right == NULL */
		} /* End of if condition current->left == NULL*/
	} /* End of while */
}


// minified obfuscated version xd fur fun
struct t{std::string n;t*l;t*r;};

void tt(t*a){t*b;while(a){if(a->l){b=a->l;for(;b->r&&b->r!=a;b=b->r);
if(b->r){b->r=0;goto l;}b->r=a;a=a->l;}else{l:printf("%s ",&a->n[0]);a=a->r;}}}

// asm version xD very funny!

#ifdef _DEBUG

void tt_moris_asm(t* tree_node) {

	const char* format_s = "%s ";


	__asm {
		mov ecx, [tree_node]
	lp:
		test ecx, ecx
		je end
		mov eax, [ecx + 1Ch] // left
		test eax, eax // if (a->l)
		je visit
	
	for_loop:
		cmp [eax + 20h], 0 // b->r != 0;
		je for_break
		cmp [eax + 20h], ecx // b->r != a;
		je for_break
		mov eax, [eax + 20h] // b = b->r
		jmp for_loop
	for_break:
		cmp [eax + 20h], 0 // b->r == 0
		je setup_link
		and dword ptr [eax + 20h], 0 // b->r = 0
		jmp visit
	setup_link:
		mov [eax + 20h], ecx; // b->r = a;
		mov ecx, [ecx + 1Ch] // a = a->l;
		jmp lp
	visit:
		mov ebx, ecx
		add ecx, 4
		push ecx
		push format_s
		call printf
		add esp, 8
		mov ecx, [ebx + 20h] // a = a->r;
		jmp lp
	end:
	}
	return;
}

#endif

#ifndef _DEBUG

void tt_moris_asm(t* tree_node) {

	const char* format_s = "%s ";


	__asm {
			mov ebx, [tree_node]
		lp:
			test ebx, ebx
			je end
			mov eax, [ebx + 18h] // left
			test eax, eax // if (a->l)
			je visit

		for_loop:
			mov edx, [eax + 1Ch]
			cmp edx, 0 // b->r != 0;
			je setup_link
			cmp edx, ebx // b->r != a;
			je for_break
			mov eax, edx // b = b->r
			jmp for_loop
		setup_link:
			mov [eax + 1Ch], ebx // b->r = a;
			mov ebx, [ebx + 18h] // a = a->l;
			jmp lp
		for_break :
			and dword ptr[eax + 1Ch], 0 // b->r = 0
		visit:
			push ebx
			push format_s
			call printf
			add esp, 8
			mov ebx, [ebx + 1Ch]// a = a->r
			jmp lp
		end:
	}
	return;
}

#endif // !_DEBUG


void tree_traverse_midorder_no_stack(Tree_node* tree_node) {
	if (!tree_node) return;

	// Well i tried :(

	Tree_node* curr = tree_node;
	Tree_node* next = tree_node->left;
	Tree_node* prev = nullptr;
	Tree_node* tmp;

	while (next) {
		if (next) {
			if (curr->left == next) {
				curr->left = next->left;
				next->left = curr;
				curr = next;
				next = curr->left;
			}

			if (curr->right == next) {
				curr->right = next->right;
				next->right = curr;
				curr = next;
				next = curr->right;
			}


		}

		if (curr->right)
		{
			next = curr->right;
			continue;
		}

		if (!curr->left && !curr->right) {
			printf("%s ", curr->name.data());

		}
	}




}

void tree_traverse_outorder(Tree_node* tree_node) {
	if (!tree_node) return;

	tree_traverse_outorder(tree_node->left);
	tree_traverse_outorder(tree_node->right);
	printf("%s ", tree_node->name.data());
}

void tree_traverse_print(Tree_node* tree_node) {
	printf("\nIn order traverse:\n");
	tree_traverse_inorder(tree_node);
	printf("\n");

	printf("\nMid order traverse:\n");
	tree_traverse_midorder(tree_node);
	printf("\n");

	printf("\nOut order traverse:\n");
	tree_traverse_outorder(tree_node);
	printf("\n");

	printf("\nMorris Traversal: \n");
	MorrisTraversal(tree_node);
	printf("\n");

	printf("\n 2: \n");
	tt((t*)tree_node);
	printf("\n");

	printf("\nIn order traverse for check:\n");
	tree_traverse_inorder(tree_node);
	printf("\n");

	printf("\ntt Asm ver \n");
	tt_moris_asm((t*)tree_node);
	printf("\n");

	printf("\nIn order traverse for check:\n");
	tree_traverse_inorder(tree_node);
	printf("\n");

}

int main(int argc, char** argv)
{
	const char* description = "A(B(C()),D(E(),F()))";

	Tree_node* tree_node = build_tree(description, strlen(description));

	tree_traverse_print(tree_node);
	
	system("PAUSE");
	return 1;
}
