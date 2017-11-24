#ifndef LINEAR_ELASTIC_SOLVER
#define LINEAR_ELASTIC_SOLVER

#include <deal.II/base/function.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/tensor.h>
#include <deal.II/base/timer.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_renumbering.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_in.h>
#include <deal.II/grid/grid_out.h>
#include <deal.II/grid/grid_refinement.h>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/constraint_matrix.h>
#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/data_postprocessor.h>
#include <deal.II/numerics/error_estimator.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/vector_tools.h>

// To transfer solutions between meshes, this file is included:
#include <deal.II/numerics/solution_transfer.h>

#include <fstream>
#include <iostream>

#include "linearElasticMaterial.h"
#include "parameters.h"
#include "utilities.h"

namespace Solid
{
  using namespace dealii;

  extern template class LinearElasticMaterial<2>;
  extern template class LinearElasticMaterial<3>;

  /*! \brief A time-dependent solver for linear elasticity.
   *
   * We use Newmark-beta method for time-stepping which can be either
   * explicit or implicit, first order accurate or second order accurate.
   * We fix \f$\beta = \frac{1}{2}\gamma\f$, which corresponds to
   * average acceleration method.
   */
  template <int dim>
  class LinearElasticSolver
  {
  public:
    /*! \brief Constructor.
     *
     * The triangulation can either be generated using dealii functions or
     * from Abaqus input file.
     * Also we use a parameter handler to specify all the input parameters.
     */
    LinearElasticSolver(Triangulation<dim> &,
                        const Parameters::AllParameters &);
    /*! \brief Destructor. */
    ~LinearElasticSolver() { dof_handler.clear(); }
    void run();

  private:
    /**
     * Set up the DofHandler, reorder the grid, sparsity pattern.
     */
    void setup_dofs();

    /**
     * Initialize the matrix, solution, and rhs. This is separated from
     * setup_dofs because if we may want to transfer solution from one grid
     * to another in the refine_mesh.
     */
    void initialize_system();

    /**
     * Assembles lhs and rhs. At time step 0, the lhs is the mass matrix;
     * at all the following steps, it is \f$ M + \beta{\Delta{t}}^2K \f$.
     */
    void assemble_system(bool is_initial);

    /**
     * Solve the linear system. Returns the number of
     * CG iterations and the final residual.
     */
    std::pair<unsigned int, double> solve(const SparseMatrix<double>&,
      Vector<double>&, const Vector<double>&);

    /**
     * Output the time-dependent solution in vtu format.
     */
    void output_results(const unsigned int) const;

    LinearElasticMaterial<dim> material;

    const double gamma; //!< Newton-beta parameter
    const double beta;  //!< Newton-beta parameter

    const unsigned int
      degree; //!< Polynomial degree, also determines quadrature order.

    const double tolerance; //!< Absolute tolerance

    Triangulation<dim> &triangulation;
    FESystem<dim> fe;
    DoFHandler<dim> dof_handler;

    const QGauss<dim>
      volume_quad_formula; //!< Quadrature formula for volume integration.

    const QGauss<dim - 1>
      face_quad_formula; //!< Quadrature formula for face integration.

    /**
     * Constraints to handle both hanging nodes and Dirichlet boundary conditions.
     */
    ConstraintMatrix constraints;

    SparsityPattern pattern;

    SparseMatrix<double> system_matrix; //!< \f$ M + \beta{\Delta{t}}^2K \f$.
    SparseMatrix<double> stiffness_matrix; //!< The stiffness is used in the rhs.
    Vector<double> system_rhs;

    /**
     * In the Newmark-beta method, acceleration is the variable to solve at
     * every
     * timestep. But displacement and velocity also contribute to the rhs of the
     * equation.
     * For the sake of clarity, we explicitly store two sets of accleration,
     * velocity
     * and displacement.
     */
    Vector<double> current_acceleration;
    Vector<double> current_velocity;
    Vector<double> current_displacement;
    Vector<double> previous_acceleration;
    Vector<double> previous_velocity;
    Vector<double> previous_displacement;

    Utils::Time time;
    mutable TimerOutput timer;
  };
}

#endif
