NAME				:= webserv
CXX					:= c++
CXX_FLAGS			:= -Wall -Wextra -Werror -std=c++17

# Sources
SOURCES				:= main.cpp \
						ConfigParser.cpp \
						ServerConfigData.cpp \
						LocationParser.cpp \
						HttpServer.cpp \
						ServerHandler.cpp \
						ServerException.cpp \
						RequestHandler.cpp \
						RequestParser.cpp \
						ResponseHandler.cpp \
						Logger.cpp \
						CGIHandler.cpp \
						FileHandler.cpp \
						DNS.cpp
						
VPATH				+= sources/ sources/Config/

# Objects
OBJECT_DIRECTORY	:= objects/
OBJECTS				:= ${SOURCES:.cpp=.o}
OBJECT_PATHS		:= $(addprefix $(OBJECT_DIRECTORY), $(OBJECTS))

# Includes
INCLUDE_DIRECTORY	:= includes/
INCLUDES			:=  RequestHandler.hpp \
						RequestParser.hpp \
						HttpResponse.hpp \
						HttpServer.hpp \
						ServerHandler.hpp \
						ServerException.hpp \
						CGIHandler.hpp \
						RequestHandler.hpp \
						ResponseHandler.hpp \
						FileHandler.hpp \
						DNS.hpp

INCLUDE_PATHS		:= $(addprefix $(INCLUDE_DIRECTORY), $(INCLUDES))
INCLUDE_FLAGS		:= -I includes/

all: $(NAME)

$(NAME): $(OBJECT_DIRECTORY) $(OBJECT_PATHS) $(INCLUDE_PATHS) Makefile
	@echo "Linking executable: $(NAME)..."
	@$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) $(OBJECT_PATHS) -o $(NAME)
	@echo "$(NAME) built with $(CXX) and flags $(CXX_FLAGS)!"

$(OBJECT_DIRECTORY):
	@mkdir -p $(OBJECT_DIRECTORY)

$(OBJECT_DIRECTORY)%.o: %.cpp $(INCLUDE_PATHS) Makefile
	@echo "Compiling $<"
	@$(CXX) $(CXX_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

clean:
	@echo "Removing object files"
	@rm -rf $(OBJECT_DIRECTORY)

fclean: clean
	@echo "Removing executable"
	@rm -f $(NAME)

re: fclean all

debug: CXX_FLAGS += -g
debug: CXX := ${CXX:c++=g++}
debug: fclean $(NAME)

sanitize: CXX_FLAGS += -g -fsanitize=address
sanitize: CXX := ${CXX:c++=g++}
sanitize: fclean $(NAME)

.PHONY: all clean fclean re debug sanitize
