/*
// $Id: //open/mondrian/src/main/mondrian/rolap/RolapAggregator.java#9 $
// This software is subject to the terms of the Common Public License
// Agreement, available at the following URL:
// http://www.opensource.org/licenses/cpl.html.
// Copyright (C) 2003-2005 Julian Hyde
// All Rights Reserved.
// You must accept the terms of that agreement to use this software.
*/
package mondrian.rolap;

import mondrian.olap.MondrianDef;
import mondrian.olap.Aggregator;
import mondrian.olap.EnumeratedValues;
import mondrian.olap.Evaluator;
import mondrian.olap.Exp;
import mondrian.olap.fun.FunUtil;

import java.util.List;

/**
 * Describes an aggregation operator, such as "sum" or "count".
 *
 * @author jhyde$
 * @since Jul 9, 2003$
 * @version $Id: //open/mondrian/src/main/mondrian/rolap/RolapAggregator.java#9 $
 */
public abstract class RolapAggregator
        extends EnumeratedValues.BasicValue
        implements Aggregator {

    private static int index = 0;

    public static final RolapAggregator Sum =
    new RolapAggregator("sum", index++, false) {
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            return FunUtil.sum(evaluator, members, exp);
        }
    };
    public static final RolapAggregator Count =
    new RolapAggregator("count", index++, false) {

        public Aggregator getRollup() {
            return Sum;
        }
       
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            return FunUtil.count(evaluator, members, false);
        }
    };
    public static final RolapAggregator Min =
    new RolapAggregator("min", index++, false) {
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            return FunUtil.min(evaluator, members, exp);
        }
    };
    public static final RolapAggregator Max =
    new RolapAggregator("max", index++, false) {
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            return FunUtil.max(evaluator, members, exp);
        }
    };
    public static final RolapAggregator Avg =
    new RolapAggregator("avg", index++, false) {
        public Aggregator getRollup() {
            return null;
        }
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            return FunUtil.avg(evaluator, members, exp);
        }
    };
    public static final RolapAggregator DistinctCount =
    new RolapAggregator("distinct count", index++, true) {
        public RolapAggregator getNonDistinctAggregator() {
            return Count;
        }
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            throw new UnsupportedOperationException();
        }

        public String getExpression(String operand) {
            return "count(distinct " + operand + ")";
        }

    };
    
    public static final RolapAggregator SeqSim = 
    new RolapAggregator("seqsim", index++, false) {
        public Aggregator getRollup() {
            return Min; // Aggregate fact tables should already have value computed
        }
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            throw new UnsupportedOperationException();
        }
    };
    
    public static final RolapAggregator SeqSimHash = 
    new RolapAggregator("seqsimhash", index++, false) {
        public Aggregator getRollup() {
            return Min; // Aggregate fact tables should already have value computed
        }
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            throw new UnsupportedOperationException();
        }
    };
    
   
    /**
     * List of all valid aggregation operators.
     */
    public static final EnumeratedValues enumeration = new EnumeratedValues (
            new RolapAggregator[] {Sum, Count, Min, Max, Avg, DistinctCount, SeqSim, SeqSimHash}
    );

    /**
     * This is the base class for implementing aggregators over sum and
     * average columns in an aggregate table. These differ from the above
     * aggregators in that these require not oly the operand to create
     * the aggregation String expression, but also, the aggregate table's
     * fact count column expression.
     * These aggregators are NOT singletons like the above aggregators; rather,
     * each is different because of the fact count column expression.
     */
    protected abstract static class BaseAggor extends RolapAggregator {
        protected final String factCountExpr;

        protected BaseAggor(final String name, final String factCountExpr) {
            super(name, index++, false);
            this.factCountExpr = factCountExpr;
        }
        public Object aggregate(Evaluator evaluator, List members, Exp exp) {
            throw new UnsupportedOperationException();
        }
    }
    /**
     * This is an aggregator used for aggregate tables implementing the
     * average aggregator. It uses the aggregate table fact_count column
     * and a sum measure to create the query used to generate an average:
     * <pre>
     *    avg == sum(column_sum) / sum(factcount).
     * </pre>
     * If the fact table has both a sum and average over the same column and
     * the aggregate table only has a sum and fact count column, then the
     * average aggregator can be generated using this aggregator.
     */
    public static class AvgFromSum extends BaseAggor {
        public AvgFromSum(String factCountExpr) {
            super("AvgFromSum", factCountExpr);
        }
        public String getExpression(String operand) {
            StringBuffer buf = new StringBuffer(64);
            buf.append("sum(");
            buf.append(operand);
            buf.append(") / sum (");
            buf.append(factCountExpr);
            buf.append(')');
            return buf.toString();
        }
    }
    /**
     * This is an aggregator used for aggregate tables implementing the
     * average aggregator. It uses the aggregate table fact_count column
     * and an average measure to create the query used to generate an average:
     * <pre>
     *    avg == sum(column_sum * factcount) / sum(factcount).
     * </pre>
     * If the fact table has both a sum and average over the same column and
     * the aggregate table only has a average and fact count column, then the
     * average aggregator can be generated using this aggregator.
     */
    public static class AvgFromAvg extends BaseAggor {
        public AvgFromAvg(String factCountExpr) {
            super("AvgFromAvg", factCountExpr);
        }
        public String getExpression(String operand) {
            StringBuffer buf = new StringBuffer(64);
            buf.append("sum(");
            buf.append(operand);
            buf.append(" * ");
            buf.append(factCountExpr);
            buf.append(") / sum (");
            buf.append(factCountExpr);
            buf.append(')');
            return buf.toString();
        }
    }
    /**
     * This is an aggregator used for aggregate tables implementing the
     * sum aggregator. It uses the aggregate table fact_count column
     * and an average measure to create the query used to generate a sum:
     * <pre>
     *    sum == sum(column_avg * factcount)
     * </pre>
     * If the fact table has both a sum and average over the same column and
     * the aggregate table only has an average and fact count column, then the
     * sum aggregator can be generated using this aggregator.
     */
    public static class SumFromAvg extends BaseAggor {
        public SumFromAvg(String factCountExpr) {
            super("SumFromAvg", factCountExpr);
        }
        public String getExpression(String operand) {
            StringBuffer buf = new StringBuffer(64);
            buf.append("sum(");
            buf.append(operand);
            buf.append(" * ");
            buf.append(factCountExpr);
            buf.append(')');
            return buf.toString();
        }
    }




    private final boolean distinct;

    public RolapAggregator(String name, int ordinal, boolean distinct) {
        super(name, ordinal, null);
        this.distinct = distinct;
    }

    public boolean isDistinct() {
        return distinct;
    }
    /**
     * Returns the expression to apply this aggregator to an operand.
     * For example, <code>getExpression("emp.sal")</code> returns
     * <code>"sum(emp.sal)"</code>.
     */
    public String getExpression(String operand) {
        StringBuffer buf = new StringBuffer(64);
        if (name.compareTo("nofn") != 0) {
        	buf.append(name);
        }
        buf.append('(');                                                                
        if (distinct) {
            buf.append("distinct ");
        }
        buf.append(operand);
        buf.append(')');
        return buf.toString();
    }
    /**
     * If this is a distinct aggregator, returns the corresponding non-distinct
     * aggregator, otherwise throws an error.
     */
    public RolapAggregator getNonDistinctAggregator() {
        throw new UnsupportedOperationException();
    }
    /**
     * Returns the aggregator used to roll up. By default, aggregators roll up
     * themselves.
     */
    public Aggregator getRollup() {
        return this;
    }
}

// End RolapAggregator.java
