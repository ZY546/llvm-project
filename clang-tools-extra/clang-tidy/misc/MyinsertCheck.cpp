//===--- MyinsertCheck.cpp - clang-tidy -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MyinsertCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceLocation.h"
#include <clang/AST/Expr.h>
#include <fstream>
#include <utility>
#include <vector>
#include <string>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void MyinsertCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(
    binaryOperator(
      hasOperatorName("+"),
      hasAncestor(
        functionDecl(hasName("test")).bind("func")
      ),
      anyOf(
        hasAncestor(
          declStmt().bind("stmt")
        ),
        hasAncestor(
          binaryOperator(hasOperatorName("=")).bind("stmt")
        )
      )
    ).bind("Op"), this);
}

VarDecl* findlvd(Expr* expr)
{
  VarDecl *ret=NULL;
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(expr)) {
  // It's a reference to a declaration...
    if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
    // It's a reference to a variable (a local, function parameter, global, or static data member).
      ret=VD;
    }
  }

  else if(BinaryOperator *BO=dyn_cast<BinaryOperator>(expr))
  {
    if("+"==BO->getOpcodeStr(BO->getOpcode()))
    {
      auto *Rhs= BO->getRHS()->IgnoreParenImpCasts();
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(Rhs)) {
      // It's a reference to a declaration...
        if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        // It's a reference to a variable (a local, function parameter, global, or static data member).
          ret=VD;
        }
      }
    }
  }
  return ret;
}
VarDecl* findrvd(Expr* expr)
{
  VarDecl *ret=NULL;
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(expr)) {
  // It's a reference to a declaration...
    if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
    // It's a reference to a variable (a local, function parameter, global, or static data member).
      ret=VD;
    }
  }
  else if(BinaryOperator *BO=dyn_cast<BinaryOperator>(expr))
  {
    if("+"==BO->getOpcodeStr(BO->getOpcode()))
    {
      auto *Lhs= BO->getLHS()->IgnoreParenImpCasts();
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(Lhs)) {
      // It's a reference to a declaration...
        if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        // It's a reference to a variable (a local, function parameter, global, or static data member).
          ret=VD;
        }
      }
    }
  }
  return ret;
}


std::vector<std::pair<VarDecl*, VarDecl*>> addpair;
int addpair_size = 0;

int num=0;
void MyinsertCheck::insertDST(const BinaryOperator* BO, VarDecl *LVD, VarDecl *RVD, SourceLocation Loc)
{
  for(int i=0;i<addpair.size();i++)
  {
    if(addpair[i]==std::make_pair(LVD,RVD))
    {
      num++;
      std::string insertcode="\nif (myinsertn" + std::to_string(i);
      insertcode+= " != " ;
      insertcode+=LVD->getQualifiedNameAsString()+" + "+RVD->getQualifiedNameAsString()
                +"){ myMark="+std::to_string(addpair_size)+"; }\n";
      diag(Loc, insertcode)
            << FixItHint::CreateInsertion(Loc, insertcode);
    }
  }
}

SourceLocation beginloc;

void MyinsertCheck::insertSRC(const BinaryOperator* BO, VarDecl *LVD, VarDecl *RVD, SourceLocation Loc)
{
  std::string str1="extern int myinsertn"+std::to_string(addpair_size)+";\n";
  std::ofstream out;
  out.open("/root/TestLlvm/CPP/dst/init.h",std::ios::app);
  out<<str1<<std::endl;
  out.close();
  //diag(beginloc, str1)
       // << FixItHint::CreateInsertion(beginloc, str1);
  std::string insertcode="\nmyinsertn" + std::to_string(addpair_size);
  insertcode+= " = " ;
  insertcode+=LVD->getQualifiedNameAsString()+" + "+RVD->getQualifiedNameAsString()+";\n";
  diag(Loc, insertcode)
        << FixItHint::CreateInsertion(Loc, insertcode);

  addpair.push_back(std::make_pair(LVD,RVD));
  addpair_size++;
}

void MyinsertCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *stmt = Result.Nodes.getNodeAs<Stmt>("stmt");
  const auto* func=Result.Nodes.getNodeAs<FunctionDecl>("func");
  
  beginloc=func->getBody()->getSourceRange().getBegin().getLocWithOffset(1);
    
  const auto *BO = Result.Nodes.getNodeAs<BinaryOperator>("Op");
  auto *Lhs= BO->getLHS()->IgnoreParenImpCasts();
  auto *Rhs= BO->getRHS()->IgnoreParenImpCasts();

  VarDecl *LVD=findlvd(Lhs);
  VarDecl *RVD=findrvd(Rhs);

  if(LVD&&RVD)
  {
    std::pair<VarDecl*, VarDecl*> thispair1=std::make_pair(LVD,RVD);
    std::pair<VarDecl*, VarDecl*> thispair2=std::make_pair(RVD,LVD);

    std::string diagLoc=BO->getOperatorLoc().printToString(Result.Context->getSourceManager());
    diag(BO->getOperatorLoc(), diagLoc);

    num=0;
    //dst
    if(std::find(addpair.begin(), addpair.end(), thispair1) != addpair.end())
    {
      auto Loc=stmt->getBeginLoc();
      insertDST(BO,LVD,RVD,Loc);
    }
    if(std::find(addpair.begin(), addpair.end(), thispair2) != addpair.end())
    {
      auto Loc=stmt->getBeginLoc();
      insertDST(BO,RVD,LVD,Loc);
    }

    if(num!=0){
      std::ofstream out;
      out.open("/root/TestLlvm/CPP/dst/out1.txt",std::ios::app);
      out<<addpair_size<<" "<<num<<" "<<diagLoc<<std::endl;
      out.close();
    }

    //插入src
    auto Loc=Lexer::getLocForEndOfToken(stmt->getEndLoc(),0,Result.Context->getSourceManager(),Result.Context->getLangOpts());
    if (((BinaryOperator*)stmt)->getOpcode()){ Loc=Loc.getLocWithOffset(1);}
    insertSRC(BO,LVD,RVD,Loc);
  }

}

} // namespace misc
} // namespace tidy
} // namespace clang
