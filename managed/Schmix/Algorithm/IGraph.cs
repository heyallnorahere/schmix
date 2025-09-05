namespace Schmix.Algorithm;

using System.Collections.Generic;

public interface IGraphVertex<T> where T : IGraphVertex<T>
{
    public int VertexID { get; }
}

public interface IGraphSolver<T> where T : IGraphVertex<T>
{
    public IReadOnlyCollection<T> Solve(IReadOnlyCollection<T> graph);
}
