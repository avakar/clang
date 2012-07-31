//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
using namespace clang;

namespace {

class RefExtractor : public RecursiveASTVisitor<RefExtractor> {
public:
	virtual bool VisitFunctionDecl(FunctionDecl * FD)
	{
		if (FD->getLinkage() == ExternalLinkage && FD->hasBody())
			exported_symbols.insert(FD->getQualifiedNameAsString());

		return true;
	}

	virtual bool VisitVarDecl(VarDecl * FD)
	{
		if (FD->getLinkage() == ExternalLinkage && FD->isThisDeclarationADefinition())
			exported_symbols.insert(FD->getQualifiedNameAsString());

		return true;
	}

	virtual bool VisitDeclRefExpr(DeclRefExpr * expr)
	{
		if (expr->getDecl()->getLinkage() == ExternalLinkage)
		{
			switch (expr->getDecl()->getKind())
			{
			default:
				break;
			case Decl::Function:
			case Decl::Var:
				referenced_symbols.insert(expr->getDecl()->getQualifiedNameAsString());
				break;
			}
		}

		return true;
	}

	void Print(llvm::raw_ostream & out)
	{
		for (std::set<std::string>::iterator iter = referenced_symbols.begin(); iter != referenced_symbols.end(); ++iter)
			out << *iter << "\n";
		out << "\n";
		for (std::set<std::string>::iterator iter = exported_symbols.begin(); iter != exported_symbols.end(); ++iter)
			out << *iter << "\n";
	}

private:
	std::set<std::string> referenced_symbols;
	std::set<std::string> exported_symbols;
};

class FakeLinkConsumer : public ASTConsumer {
public:
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
	  RefExtractor extractor;
	  extractor.TraverseDecl(Ctx.getTranslationUnitDecl());
	  extractor.Print(llvm::outs());
  }
};

class FakeLinkAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    return new FakeLinkConsumer();
  }

  virtual bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &arg) {
    return true;
  }
};

}

static FrontendPluginRegistry::Add<FakeLinkAction>
	X("fake-link", "produce a list of exported/imported symbols");
