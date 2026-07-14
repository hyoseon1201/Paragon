package com.p1.backend.service;

import com.p1.backend.entity.User;
import com.p1.backend.repository.UserRepository;
import org.mindrot.jbcrypt.BCrypt;
import org.springframework.stereotype.Service;

@Service
public class AuthService {

    private final UserRepository userRepository;
    private final JwtService jwtService;

    public AuthService(UserRepository userRepository, JwtService jwtService) {
        this.userRepository = userRepository;
        this.jwtService = jwtService;
    }

    public static class UsernameTakenException extends RuntimeException {
    }

    public static class InvalidCredentialsException extends RuntimeException {
    }

    public void signup(String username, String rawPassword) {
        if (userRepository.existsByUsername(username)) {
            throw new UsernameTakenException();
        }
        String hash = BCrypt.hashpw(rawPassword, BCrypt.gensalt());
        userRepository.save(new User(username, hash));
    }

    public String login(String username, String rawPassword) {
        User user = userRepository.findByUsername(username)
                .orElseThrow(InvalidCredentialsException::new);
        if (!BCrypt.checkpw(rawPassword, user.getPasswordHash())) {
            throw new InvalidCredentialsException();
        }
        return jwtService.generateToken(username);
    }
}
