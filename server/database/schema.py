from sqlalchemy import create_engine, Column, Integer, String
from sqlalchemy.orm import declarative_base, sessionmaker

DATABASE_URL = "postgresql://LED_user:LED_password@localhost:5432/LED_database"

engine = create_engine(DATABASE_URL)
Session = sessionmaker(bind=engine)
session = Session()
Base = declarative_base()

class User(Base):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, autoincrement=True)
    username = Column(String, index=True, nullable=False)
    hashed_password = Column(String, nullable=False)

    def __repr__(self):
        return f"<User(username={self.username})>"


def create_schema():
    Base.metadata.create_all(engine)
    print("Users table created successfully.")

def add_user(username, password):
    user = User(username=username, hashed_password=password)
    session.add(user)
    session.commit()
    print(f"Added user: {username}")

def remove_user(username):
    user = session.query(User).filter_by(username=username).first()
    if user:
        session.delete(user)
        session.commit()
        print(f"Removed user: {username}")
    else:
        print(f"User '{username}' not found.")

if __name__ == "__main__":
    create_schema()

# <demonstration>, this is temporary
    add_user("Jacob", "hashed_pass_251")
    add_user("nu_user", "hashed_pass_812")
    add_user("frank_21", "hashed_pass_213")

    remove_user("Jacob")
