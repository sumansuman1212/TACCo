#ifndef TACCO_DECLARATIONS_HPP
#define TACCO_DECLARATIONS_HPP

#include "ast.hpp"
#include "statements.hpp"
#include <string>
#include <vector>
#include <memory>

namespace tacco {

// Variable declaration: type name [size]? (= init)?;
class VariableDeclarationNode : public DeclarationNode {
public:
    TypePtr type;
    std::string name;
    std::unique_ptr<ExpressionNode> initializer; // May be nullptr
    int array_size = 0;
    bool is_array = false;

    VariableDeclarationNode(TypePtr t, std::string n,
                            std::unique_ptr<ExpressionNode> init,
                            int arr_sz, bool is_arr,
                            SourceLocation loc)
        : DeclarationNode(std::move(loc)),
          type(std::move(t)),
          name(std::move(n)),
          initializer(std::move(init)),
          array_size(arr_sz),
          is_array(is_arr) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Parameter: type name
class ParameterNode : public ASTNode {
public:
    TypePtr type;
    std::string name;

    ParameterNode(TypePtr t, std::string n, SourceLocation loc)
        : ASTNode(std::move(loc)), type(std::move(t)), name(std::move(n)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Function declaration: type name(params...);
class FunctionDeclarationNode : public DeclarationNode {
public:
    TypePtr return_type;
    std::string name;
    std::vector<std::unique_ptr<ParameterNode>> params;

    FunctionDeclarationNode(TypePtr ret, std::string n,
                            std::vector<std::unique_ptr<ParameterNode>> p,
                            SourceLocation loc)
        : DeclarationNode(std::move(loc)),
          return_type(std::move(ret)),
          name(std::move(n)),
          params(std::move(p)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Function definition: type name(params...) { body }
class FunctionDefinitionNode : public DeclarationNode {
public:
    TypePtr return_type;
    std::string name;
    std::vector<std::unique_ptr<ParameterNode>> params;
    std::unique_ptr<CompoundStatementNode> body;

    FunctionDefinitionNode(TypePtr ret, std::string n,
                           std::vector<std::unique_ptr<ParameterNode>> p,
                           std::unique_ptr<CompoundStatementNode> b,
                           SourceLocation loc)
        : DeclarationNode(std::move(loc)),
          return_type(std::move(ret)),
          name(std::move(n)),
          params(std::move(p)),
          body(std::move(b)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Class field declaration: type name [size]?;
class FieldDeclarationNode : public ASTNode {
public:
    TypePtr type;
    std::string name;
    int array_size = 0;
    bool is_array = false;

    FieldDeclarationNode(TypePtr t, std::string n, int arr_sz, bool is_arr, SourceLocation loc)
        : ASTNode(std::move(loc)), type(std::move(t)), name(std::move(n)),
          array_size(arr_sz), is_array(is_arr) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Class declaration: class Name { fields... };
class ClassDeclarationNode : public DeclarationNode {
public:
    std::string name;
    std::vector<std::unique_ptr<FieldDeclarationNode>> fields;

    ClassDeclarationNode(std::string n,
                         std::vector<std::unique_ptr<FieldDeclarationNode>> f,
                         SourceLocation loc)
        : DeclarationNode(std::move(loc)), name(std::move(n)), fields(std::move(f)) {}

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

// Program: Root AST container
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<DeclarationNode>> declarations;

    explicit ProgramNode(SourceLocation loc) : ASTNode(std::move(loc)) {}

    void add_declaration(std::unique_ptr<DeclarationNode> decl) {
        declarations.push_back(std::move(decl));
    }

    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
    void dump(std::ostream& os, int indent = 0) const override;
};

} // namespace tacco

#endif // TACCO_DECLARATIONS_HPP
