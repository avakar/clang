//===-- Internals.h - Implementation Details---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_ARCMIGRATE_INTERNALS_H
#define LLVM_CLANG_LIB_ARCMIGRATE_INTERNALS_H

#include "clang/ARCMigrate/ARCMT.h"
#include "llvm/ADT/ArrayRef.h"

namespace clang {
  class Sema;
  class Stmt;

namespace arcmt {

class CapturedDiagList {
  typedef std::list<StoredDiagnostic> ListTy;
  ListTy List;
  
public:
  void push_back(const StoredDiagnostic &diag) { List.push_back(diag); }

  bool clearDiagnostic(ArrayRef<unsigned> IDs, SourceRange range);
  bool hasDiagnostic(ArrayRef<unsigned> IDs, SourceRange range) const;

  void reportDiagnostics(DiagnosticsEngine &diags) const;

  bool hasErrors() const;

  typedef ListTy::const_iterator iterator;
  iterator begin() const { return List.begin(); }
  iterator end()   const { return List.end();   }
};

void writeARCDiagsToPlist(const std::string &outPath,
                          ArrayRef<StoredDiagnostic> diags,
                          SourceManager &SM, const LangOptions &LangOpts);

class TransformActions {
  DiagnosticsEngine &Diags;
  CapturedDiagList &CapturedDiags;
  bool ReportedErrors;
  void *Impl; // TransformActionsImpl.

public:
  TransformActions(DiagnosticsEngine &diag, CapturedDiagList &capturedDiags,
                   ASTContext &ctx, Preprocessor &PP);
  ~TransformActions();

  void startTransaction();
  bool commitTransaction();
  void abortTransaction();

  void insert(SourceLocation loc, StringRef text);
  void insertAfterToken(SourceLocation loc, StringRef text);
  void remove(SourceRange range);
  void removeStmt(Stmt *S);
  void replace(SourceRange range, StringRef text);
  void replace(SourceRange range, SourceRange replacementRange);
  void replaceStmt(Stmt *S, StringRef text);
  void replaceText(SourceLocation loc, StringRef text,
                   StringRef replacementText);
  void increaseIndentation(SourceRange range,
                           SourceLocation parentIndent);

  bool clearDiagnostic(ArrayRef<unsigned> IDs, SourceRange range);
  bool clearAllDiagnostics(SourceRange range) {
    return clearDiagnostic(ArrayRef<unsigned>(), range);
  }
  bool clearDiagnostic(unsigned ID1, unsigned ID2, SourceRange range) {
    unsigned IDs[] = { ID1, ID2 };
    return clearDiagnostic(IDs, range);
  }
  bool clearDiagnostic(unsigned ID1, unsigned ID2, unsigned ID3,
                       SourceRange range) {
    unsigned IDs[] = { ID1, ID2, ID3 };
    return clearDiagnostic(IDs, range);
  }

  bool hasDiagnostic(unsigned ID, SourceRange range) {
    return CapturedDiags.hasDiagnostic(ID, range);
  }

  bool hasDiagnostic(unsigned ID1, unsigned ID2, SourceRange range) {
    unsigned IDs[] = { ID1, ID2 };
    return CapturedDiags.hasDiagnostic(IDs, range);
  }

  void reportError(StringRef error, SourceLocation loc,
                   SourceRange range = SourceRange());
  void reportNote(StringRef note, SourceLocation loc,
                  SourceRange range = SourceRange());

  bool hasReportedErrors() const { return ReportedErrors; }

  class RewriteReceiver {
  public:
    virtual ~RewriteReceiver();

    virtual void insert(SourceLocation loc, StringRef text) = 0;
    virtual void remove(CharSourceRange range) = 0;
    virtual void increaseIndentation(CharSourceRange range,
                                     SourceLocation parentIndent) = 0;
  };

  void applyRewrites(RewriteReceiver &receiver);
};

class Transaction {
  TransformActions &TA;
  bool Aborted;

public:
  Transaction(TransformActions &TA) : TA(TA), Aborted(false) {
    TA.startTransaction();
  }

  ~Transaction() {
    if (!isAborted())
      TA.commitTransaction();
  }

  void abort() {
    TA.abortTransaction();
    Aborted = true;
  }

  bool isAborted() const { return Aborted; }
};

class MigrationPass {
public:
  ASTContext &Ctx;
  Sema &SemaRef;
  TransformActions &TA;
  std::vector<SourceLocation> &ARCMTMacroLocs;

  MigrationPass(ASTContext &Ctx, Sema &sema, TransformActions &TA,
                std::vector<SourceLocation> &ARCMTMacroLocs)
    : Ctx(Ctx), SemaRef(sema), TA(TA), ARCMTMacroLocs(ARCMTMacroLocs) { }
};

static inline StringRef getARCMTMacroName() {
  return "__IMPL_ARCMT_REMOVED_EXPR__";
}

} // end namespace arcmt

} // end namespace clang

#endif
