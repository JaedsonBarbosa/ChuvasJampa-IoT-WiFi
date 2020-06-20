class SuperEstado {
    public:
    virtual void Iniciar();
    virtual void Encerrar();
    
    // O retorno informa se este estado ainda pode rodar novamente
    virtual void Executar();
};
