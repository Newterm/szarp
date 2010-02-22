#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>

namespace lua_grammar {

	namespace qi = boost::spirit::qi;

	struct expression;
	struct unop_expression;
	struct binop_expression;

	enum binop {
		PLUS,
		MINUS,
		LT,
		LTE,
		GT,
		GTE,
		EQ,
		NEQ,
		CONCAT,
		MUL,
		DIV,
		REM,
		POW,
		OR,
		AND
	};

	enum unop {
		NEG,
		NOT,	
		LEN
	};

	struct nil { std::string nil_; nil() : nil_("nil") {} };
	struct threedots { std::string threedots_; threedots() : threedots_("threedots") {} };

	typedef std::string identifier;

	struct namelist {
		namelist& operator=(const namelist& v);
		namelist& operator=(const std::vector<std::string>& v);
		std::vector<std::string> namelist_;
	};

	struct parlist1 {
		namelist namelist_;
		boost::optional<threedots> threedots_;
	};

	typedef boost::variant<
		parlist1,
		threedots> parlist;

	struct chunk;

	struct block {
		block() {};
		block& operator=(const boost::recursive_wrapper<chunk>& c);
		boost::recursive_wrapper<chunk> chunk_;
	};

	struct expression;
	typedef boost::recursive_wrapper<expression> expf;

	typedef boost::variant<
		boost::recursive_wrapper<boost::tuple<expression, expression> >,
		boost::recursive_wrapper<boost::tuple<identifier, expression> >,
		expf> field;

	struct tableconstructor {
		std::vector<field> tableconstructor_;
		tableconstructor& operator=(const std::vector<field>& v);
		tableconstructor& operator=(const tableconstructor& v);
	};

	typedef boost::variant<
		boost::recursive_wrapper<std::vector<expression> >,
		tableconstructor,
		std::string> args;

	typedef boost::variant<
		expf,
		identifier> exp_identifier;

	typedef boost::variant<
		args,
		boost::tuple<identifier, args> > namearg;

	struct var_seq {
		std::vector<namearg> nameargs;
		exp_identifier exp_identifier_;
	};

	struct varc {
		exp_identifier exp_identifier_;
		std::vector<var_seq> var_seqs;
	};

	typedef boost::variant<
		identifier,
		varc> var;

	struct functioncall_seq {
		std::vector<exp_identifier> exp_identifiers;
		namearg namearg_;
	};

	struct functioncall {
		exp_identifier exp_identifier_;
		std::vector<functioncall_seq> functioncall_seqs;
	};

	struct funcbody {
		parlist parlist_;
		block block_;
	};

	typedef boost::variant<
		nil,
		bool,
		double,
		std::string,
		threedots,
		funcbody,
		var,
		functioncall,
		tableconstructor,
		boost::recursive_wrapper<unop_expression>
		> exp_left;

	typedef boost::recursive_wrapper<binop_expression> exp_rest;

	struct expression {
		exp_left exp_left_;
		boost::optional<exp_rest> exp_rest_;
	};

	struct while_loop {
		expression expression_;
		block block_;
	};

	struct repeat_loop {
		block block_;
		expression expression_;
	};

	struct unop_expression {
		unop op;
		expression expression_;
	};

	struct binop_expression {
		binop op;
		expression expression_;
	};

	struct assignment {
		std::vector<var> varlist;
		std::vector<expression> explist;
	};

	typedef std::vector<boost::tuple<expression, block> > elseif;

	struct if_stat {
		expression if_exp;
		block block_;
		elseif elseif_;
		boost::optional<block> else_;
	};

	struct for_from_to_loop {
		identifier identifier_;
		expression from;
		expression to;
		boost::optional<expression> step;
		block block_;
	};

	struct for_in_loop {
		namelist namelist_;
		std::vector<expression> expressions;
		block block_;
	};

	struct funcname {
		std::vector<identifier> identifiers;
		boost::optional<identifier> method_name;
	};

	struct function_declaration {
		funcname funcname_;
		funcbody funcbody_;
	};

	struct local_assignment : assignment {};

	struct local_function_declaration : public function_declaration { };

	struct return_ {
		return_() {}
		return_(std::vector<expression>& v) : expressions(v) {}
		std::vector<expression> expressions;
	};

	struct break_ { };

	typedef boost::variant<
		assignment,
		functioncall,
		var,
		block,
		while_loop,
		repeat_loop,
		if_stat,
		for_in_loop,
		for_from_to_loop,
		function_declaration,
		local_assignment,
		local_function_declaration> stat;

	typedef boost::variant<
		return_,
		break_> laststat;

	struct chunk {
		std::vector<stat> stats;
		boost::optional<laststat> laststat_;	
	};

	bool parse(std::string::const_iterator& iter, std::string::const_iterator &end, chunk& chunk_);
};

